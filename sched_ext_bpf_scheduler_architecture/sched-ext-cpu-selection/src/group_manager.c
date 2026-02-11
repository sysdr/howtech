// SPDX-License-Identifier: GPL-2.0
/* Group Manager - Assigns tasks to scheduler groups */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <bpf/bpf.h>
#include <bpf/libbpf.h>

int main(int argc, char **argv)
{
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <group_map_fd> <pid> <group_id>\n", argv[0]);
        fprintf(stderr, "  group_id: 0=interactive, 1=batch\n");
        return 1;
    }
    
    int map_fd = atoi(argv[1]);
    unsigned int pid = atoi(argv[2]);
    unsigned int group_id = atoi(argv[3]);
    
    if (group_id > 1) {
        fprintf(stderr, "Invalid group_id: %u (must be 0 or 1)\n", group_id);
        return 1;
    }
    
    if (bpf_map_update_elem(map_fd, &pid, &group_id, BPF_ANY) != 0) {
        perror("bpf_map_update_elem");
        return 1;
    }
    
    const char *group_name = (group_id == 0) ? "interactive" : "batch";
    printf("Assigned PID %u to group %u (%s)\n", pid, group_id, group_name);
    
    return 0;
}
