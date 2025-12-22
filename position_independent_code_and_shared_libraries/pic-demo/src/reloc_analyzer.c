#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <elf.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

void analyze_relocations(const char *filename) {
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
    
    void *map = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (map == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return;
    }
    
    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)map;
    
    if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0) {
        fprintf(stderr, "Not a valid ELF file\n");
        munmap(map, st.st_size);
        close(fd);
        return;
    }
    
    printf("\n%-25s | %-18s | %-12s | %s\n", "RELOCATION TYPE", "OFFSET", "SYMBOL", "ADDEND");
    printf("─────────────────────────┼────────────────────┼──────────────┼──────────\n");
    
    Elf64_Shdr *shdr = (Elf64_Shdr *)((char *)map + ehdr->e_shoff);
    
    for (int i = 0; i < ehdr->e_shnum; i++) {
        if (shdr[i].sh_type == SHT_RELA) {
            Elf64_Rela *rela = (Elf64_Rela *)((char *)map + shdr[i].sh_offset);
            int num_rela = shdr[i].sh_size / sizeof(Elf64_Rela);
            
            for (int j = 0; j < num_rela && j < 20; j++) {
                const char *type_name = "UNKNOWN";
                switch (ELF64_R_TYPE(rela[j].r_info)) {
                    case R_X86_64_RELATIVE:  type_name = "R_X86_64_RELATIVE"; break;
                    case R_X86_64_GLOB_DAT:  type_name = "R_X86_64_GLOB_DAT"; break;
                    case R_X86_64_JUMP_SLOT: type_name = "R_X86_64_JUMP_SLOT"; break;
                    case R_X86_64_64:        type_name = "R_X86_64_64"; break;
                    default: break;
                }
                
                printf("%-25s | 0x%016lx | %-12ld | %ld\n",
                       type_name,
                       rela[j].r_offset,
                       ELF64_R_SYM(rela[j].r_info),
                       rela[j].r_addend);
            }
        }
    }
    
    munmap(map, st.st_size);
    close(fd);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <elf-file>\n", argv[0]);
        return 1;
    }
    
    printf("\n╔═══════════════════════════════════════════════════════╗\n");
    printf("║     ELF RELOCATION ANALYSIS: %s\n", argv[1]);
    printf("╚═══════════════════════════════════════════════════════╝\n");
    
    analyze_relocations(argv[1]);
    return 0;
}
