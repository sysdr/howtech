#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <elf.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define COLOR_BLUE    "\033[34m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_RESET   "\033[0m"

void print_section(const char* label, const char* value) {
    printf(COLOR_CYAN "  %-25s" COLOR_RESET ": %s\n", label, value);
}

void inspect_elf(const char* filename) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return;
    }
    
    struct stat st;
    if (fstat(fd, &st) < 0) {
        perror("fstat");
        close(fd);
        return;
    }
    
    void* map = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (map == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return;
    }
    
    Elf64_Ehdr* ehdr = (Elf64_Ehdr*)map;
    
    // Verify ELF magic
    if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0) {
        printf("Not an ELF file\n");
        munmap(map, st.st_size);
        close(fd);
        return;
    }
    
    printf(COLOR_BLUE "\n=== ELF Header Analysis: %s ===" COLOR_RESET "\n", filename);
    
    char class[32];
    sprintf(class, "%s", ehdr->e_ident[EI_CLASS] == ELFCLASS64 ? "64-bit" : "32-bit");
    print_section("Class", class);
    
    char type[32];
    switch(ehdr->e_type) {
        case ET_EXEC: sprintf(type, "EXEC (Executable)"); break;
        case ET_DYN:  sprintf(type, "DYN (Shared object/PIE)"); break;
        case ET_REL:  sprintf(type, "REL (Relocatable)"); break;
        default:      sprintf(type, "Unknown (%d)", ehdr->e_type); break;
    }
    print_section("Type", type);
    
    char entry[32];
    sprintf(entry, "0x%lx", ehdr->e_entry);
    print_section("Entry point", entry);
    
    char phdr_info[64];
    sprintf(phdr_info, "%d segments at offset 0x%lx", 
            ehdr->e_phnum, ehdr->e_phoff);
    print_section("Program headers", phdr_info);
    
    // Analyze program headers
    Elf64_Phdr* phdr = (Elf64_Phdr*)((char*)map + ehdr->e_phoff);
    
    printf(COLOR_GREEN "\n  Program Headers (Segments for Loading):" COLOR_RESET "\n");
    for (int i = 0; i < ehdr->e_phnum; i++) {
        char seg_type[16];
        switch(phdr[i].p_type) {
            case PT_LOAD:    strcpy(seg_type, "LOAD"); break;
            case PT_DYNAMIC: strcpy(seg_type, "DYNAMIC"); break;
            case PT_INTERP:  strcpy(seg_type, "INTERP"); break;
            case PT_GNU_RELRO: strcpy(seg_type, "GNU_RELRO"); break;
            case PT_GNU_STACK: strcpy(seg_type, "GNU_STACK"); break;
            default:         sprintf(seg_type, "0x%x", phdr[i].p_type); break;
        }
        
        char perms[4] = "---";
        if (phdr[i].p_flags & PF_R) perms[0] = 'r';
        if (phdr[i].p_flags & PF_W) perms[1] = 'w';
        if (phdr[i].p_flags & PF_X) perms[2] = 'x';
        
        printf("    %-12s VirtAddr: 0x%08lx  FileSize: 0x%06lx  Perm: %s\n",
               seg_type, phdr[i].p_vaddr, phdr[i].p_filesz, perms);
    }
    
    munmap(map, st.st_size);
    close(fd);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <elf-binary>\n", argv[0]);
        return 1;
    }
    
    for (int i = 1; i < argc; i++) {
        inspect_elf(argv[i]);
    }
    
    return 0;
}
