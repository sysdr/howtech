// SPDX-License-Identifier: GPL-2.0
/* Helper program to get BPF map file descriptor by map ID */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <bpf/bpf.h>
#include <bpf/libbpf.h>

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <map_id>\n", argv[0]);
        return 1;
    }
    
    unsigned int map_id = atoi(argv[1]);
    int map_fd = bpf_map_get_fd_by_id(map_id);
    
    if (map_fd < 0) {
        perror("bpf_map_get_fd_by_id");
        return 1;
    }
    
    printf("%d\n", map_fd);
    return 0;
}
