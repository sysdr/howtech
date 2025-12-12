#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <elf.h>
#include <errno.h>

#define COLOR_HEADER  "\033[1;36m"
#define COLOR_SECTION "\033[1;33m"
#define COLOR_SYMBOL  "\033[1;32m"
#define COLOR_RELOC   "\033[1;35m"
#define COLOR_RESET   "\033[0m"
#define COLOR_VALUE   "\033[0;37m"
#define COLOR_ADDR    "\033[0;34m"

typedef struct {
    void *data;
    size_t size;
    Elf64_Ehdr *ehdr;
    Elf64_Phdr *phdr;
    Elf64_Shdr *shdr;
    char *shstrtab;
} ElfFile;

static int elf_open(const char *path, ElfFile *elf) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return -1;
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
        perror("fstat");
        close(fd);
        return -1;
    }

    void *data = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return -1;
    }
    close(fd);

    elf->data = data;
    elf->size = st.st_size;
    elf->ehdr = (Elf64_Ehdr *)data;

    // Validate ELF magic
    if (memcmp(elf->ehdr->e_ident, ELFMAG, SELFMAG) != 0) {
        fprintf(stderr, "Not an ELF file\n");
        munmap(data, st.st_size);
        return -1;
    }

    elf->phdr = (Elf64_Phdr *)((char *)data + elf->ehdr->e_phoff);
    elf->shdr = (Elf64_Shdr *)((char *)data + elf->ehdr->e_shoff);
    elf->shstrtab = (char *)data + elf->shdr[elf->ehdr->e_shstrndx].sh_offset;

    return 0;
}

static void elf_close(ElfFile *elf) {
    if (elf->data != NULL) {
        munmap(elf->data, elf->size);
        elf->data = NULL;
    }
}

static void print_elf_header(ElfFile *elf) {
    Elf64_Ehdr *eh = elf->ehdr;
    
    printf("\n%s╔═══════════════════════════════════════════════════════════╗%s\n",
           COLOR_HEADER, COLOR_RESET);
    printf("%s║                    ELF HEADER                             ║%s\n",
           COLOR_HEADER, COLOR_RESET);
    printf("%s╚═══════════════════════════════════════════════════════════╝%s\n",
           COLOR_HEADER, COLOR_RESET);
    
    printf("  Magic:       %s%02x %02x %02x %02x%s (", COLOR_VALUE,
           eh->e_ident[0], eh->e_ident[1], eh->e_ident[2], eh->e_ident[3], COLOR_RESET);
    for (int i = 0; i < 4; i++)
        printf("%c", eh->e_ident[i] == 0x7f ? '.' : eh->e_ident[i]);
    printf(")\n");
    
    printf("  Class:       %s%s%s\n", COLOR_VALUE,
           eh->e_ident[EI_CLASS] == ELFCLASS64 ? "ELF64" : "ELF32", COLOR_RESET);
    printf("  Data:        %s%s%s\n", COLOR_VALUE,
           eh->e_ident[EI_DATA] == ELFDATA2LSB ? "Little-endian" : "Big-endian", COLOR_RESET);
    printf("  Type:        %s%s%s\n", COLOR_VALUE,
           eh->e_type == ET_EXEC ? "EXEC (Executable)" :
           eh->e_type == ET_DYN ? "DYN (Shared object)" : "Other", COLOR_RESET);
    printf("  Machine:     %s%s%s\n", COLOR_VALUE,
           eh->e_machine == EM_X86_64 ? "x86-64" : "Other", COLOR_RESET);
    printf("  Entry:       %s0x%lx%s\n", COLOR_ADDR, eh->e_entry, COLOR_RESET);
    printf("  Prog Hdrs:   %s%d%s (offset 0x%lx)\n", COLOR_VALUE,
           eh->e_phnum, COLOR_RESET, eh->e_phoff);
    printf("  Sect Hdrs:   %s%d%s (offset 0x%lx)\n", COLOR_VALUE,
           eh->e_shnum, COLOR_RESET, eh->e_shoff);
}

static void print_program_headers(ElfFile *elf) {
    printf("\n%s╔═══════════════════════════════════════════════════════════╗%s\n",
           COLOR_HEADER, COLOR_RESET);
    printf("%s║              PROGRAM HEADERS (Runtime)                    ║%s\n",
           COLOR_HEADER, COLOR_RESET);
    printf("%s╚═══════════════════════════════════════════════════════════╝%s\n",
           COLOR_HEADER, COLOR_RESET);
    
    printf("\n  %s%-14s %-10s %-16s %-8s %-8s%s\n",
           COLOR_SECTION, "Type", "Offset", "VirtAddr", "FileSz", "Flags", COLOR_RESET);
    printf("  %s", COLOR_SECTION);
    for (int i = 0; i < 70; i++) printf("─");
    printf("%s\n", COLOR_RESET);
    
    for (int i = 0; i < elf->ehdr->e_phnum; i++) {
        Elf64_Phdr *ph = &elf->phdr[i];
        const char *type = 
            ph->p_type == PT_LOAD ? "LOAD" :
            ph->p_type == PT_DYNAMIC ? "DYNAMIC" :
            ph->p_type == PT_INTERP ? "INTERP" :
            ph->p_type == PT_GNU_RELRO ? "GNU_RELRO" : "Other";
        
        char flags[4];
        snprintf(flags, sizeof(flags), "%c%c%c",
                 ph->p_flags & PF_R ? 'R' : '-',
                 ph->p_flags & PF_W ? 'W' : '-',
                 ph->p_flags & PF_X ? 'X' : '-');
        
        printf("  %s%-14s%s 0x%-8lx %s0x%014lx%s %-8ld %s%-8s%s\n",
               COLOR_VALUE, type, COLOR_RESET,
               ph->p_offset,
               COLOR_ADDR, ph->p_vaddr, COLOR_RESET,
               ph->p_filesz,
               COLOR_VALUE, flags, COLOR_RESET);
    }
}

static void print_section_headers(ElfFile *elf) {
    printf("\n%s╔═══════════════════════════════════════════════════════════╗%s\n",
           COLOR_HEADER, COLOR_RESET);
    printf("%s║               SECTION HEADERS (Linking)                   ║%s\n",
           COLOR_HEADER, COLOR_RESET);
    printf("%s╚═══════════════════════════════════════════════════════════╝%s\n",
           COLOR_HEADER, COLOR_RESET);
    
    printf("\n  %s%-20s %-12s %-16s %-10s%s\n",
           COLOR_SECTION, "Name", "Type", "Address", "Size", COLOR_RESET);
    printf("  %s", COLOR_SECTION);
    for (int i = 0; i < 70; i++) printf("─");
    printf("%s\n", COLOR_RESET);
    
    for (int i = 0; i < elf->ehdr->e_shnum; i++) {
        Elf64_Shdr *sh = &elf->shdr[i];
        const char *name = elf->shstrtab + sh->sh_name;
        
        const char *type =
            sh->sh_type == SHT_PROGBITS ? "PROGBITS" :
            sh->sh_type == SHT_SYMTAB ? "SYMTAB" :
            sh->sh_type == SHT_DYNSYM ? "DYNSYM" :
            sh->sh_type == SHT_STRTAB ? "STRTAB" :
            sh->sh_type == SHT_RELA ? "RELA" :
            sh->sh_type == SHT_HASH ? "HASH" :
            sh->sh_type == SHT_DYNAMIC ? "DYNAMIC" :
            sh->sh_type == SHT_NOTE ? "NOTE" :
            sh->sh_type == SHT_NOBITS ? "NOBITS" :
            sh->sh_type == SHT_REL ? "REL" : "Other";
        
        if (strlen(name) > 0 && sh->sh_size > 0) {
            printf("  %s%-20s%s %-12s %s0x%014lx%s %-10ld\n",
                   COLOR_VALUE, name, COLOR_RESET,
                   type,
                   COLOR_ADDR, sh->sh_addr, COLOR_RESET,
                   sh->sh_size);
        }
    }
}

static void print_dynamic_symbols(ElfFile *elf) {
    Elf64_Shdr *dynsym_sh = NULL;
    Elf64_Shdr *dynstr_sh = NULL;
    
    // Find .dynsym and .dynstr sections
    for (int i = 0; i < elf->ehdr->e_shnum; i++) {
        const char *name = elf->shstrtab + elf->shdr[i].sh_name;
        if (strcmp(name, ".dynsym") == 0) {
            dynsym_sh = &elf->shdr[i];
        } else if (strcmp(name, ".dynstr") == 0) {
            dynstr_sh = &elf->shdr[i];
        }
    }
    
    if (!dynsym_sh || !dynstr_sh) {
        printf("\n  %sNo dynamic symbols found%s\n", COLOR_VALUE, COLOR_RESET);
        return;
    }
    
    printf("\n%s╔═══════════════════════════════════════════════════════════╗%s\n",
           COLOR_HEADER, COLOR_RESET);
    printf("%s║                  DYNAMIC SYMBOLS                          ║%s\n",
           COLOR_HEADER, COLOR_RESET);
    printf("%s╚═══════════════════════════════════════════════════════════╝%s\n",
           COLOR_HEADER, COLOR_RESET);
    
    Elf64_Sym *syms = (Elf64_Sym *)((char *)elf->data + dynsym_sh->sh_offset);
    char *strtab = (char *)elf->data + dynstr_sh->sh_offset;
    int nsyms = dynsym_sh->sh_size / sizeof(Elf64_Sym);
    
    printf("\n  %s%-30s %-10s %-8s %-10s%s\n",
           COLOR_SECTION, "Name", "Type", "Bind", "Value", COLOR_RESET);
    printf("  %s", COLOR_SECTION);
    for (int i = 0; i < 70; i++) printf("─");
    printf("%s\n", COLOR_RESET);
    
    int count = 0;
    for (int i = 0; i < nsyms && count < 20; i++) {
        Elf64_Sym *sym = &syms[i];
        if (sym->st_name == 0) continue;
        
        const char *name = strtab + sym->st_name;
        const char *type = 
            ELF64_ST_TYPE(sym->st_info) == STT_FUNC ? "FUNC" :
            ELF64_ST_TYPE(sym->st_info) == STT_OBJECT ? "OBJECT" :
            ELF64_ST_TYPE(sym->st_info) == STT_NOTYPE ? "NOTYPE" : "Other";
        
        const char *bind =
            ELF64_ST_BIND(sym->st_info) == STB_GLOBAL ? "GLOBAL" :
            ELF64_ST_BIND(sym->st_info) == STB_WEAK ? "WEAK" :
            ELF64_ST_BIND(sym->st_info) == STB_LOCAL ? "LOCAL" : "Other";
        
        printf("  %s%-30s%s %-10s %-8s %s0x%08lx%s\n",
               COLOR_SYMBOL, name, COLOR_RESET, type, bind,
               COLOR_ADDR, sym->st_value, COLOR_RESET);
        count++;
    }
    
    if (nsyms > 20) {
        printf("  %s... (%d more symbols)%s\n", COLOR_VALUE, nsyms - 20, COLOR_RESET);
    }
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <elf-file>\n", argv[0]);
        return 1;
    }
    
    ElfFile elf = {0};
    if (elf_open(argv[1], &elf) < 0) {
        return 1;
    }
    
    printf("\n%s", COLOR_HEADER);
    printf("╔═════════════════════════════════════════════════════════════╗\n");
    printf("║  ELF INTERNALS ANALYZER - Deep Dive into Binary Format     ║\n");
    printf("║  File: %-52s ║\n", argv[1]);
    printf("╚═════════════════════════════════════════════════════════════╝\n");
    printf("%s", COLOR_RESET);
    
    print_elf_header(&elf);
    print_program_headers(&elf);
    print_section_headers(&elf);
    print_dynamic_symbols(&elf);
    
    printf("\n%s", COLOR_HEADER);
    printf("╔═════════════════════════════════════════════════════════════╗\n");
    printf("║  Key Takeaway: Program headers matter at runtime,          ║\n");
    printf("║  section headers matter at link time. Strip removes        ║\n");
    printf("║  sections but keeps program headers!                       ║\n");
    printf("╚═════════════════════════════════════════════════════════════╝\n");
    printf("%s\n", COLOR_RESET);
    
    elf_close(&elf);
    return 0;
}
