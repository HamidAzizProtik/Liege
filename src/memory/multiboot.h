#ifndef MULTIBOOT_H
#define MULTIBOOT_H

typedef struct multiboot_info {
    unsigned int flags;
    unsigned int mem_lower;
    unsigned int mem_upper;
    unsigned int boot_device;
    unsigned int cmdline;
    unsigned int mods_count;
    unsigned int mods_addr;
    unsigned int syms[4];
    unsigned int mmap_length;
    unsigned int mmap_addr;
} multiboot_info_t;

typedef struct multiboot_memory_map {
    unsigned int size;
    unsigned long long addr;
    unsigned long long len;
    unsigned int type;
} multiboot_memory_map_t;

#endif