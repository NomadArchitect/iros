#ifndef _KERNEL_MEM_VM_ALLOCATOR_H
#define _KERNEL_MEM_VM_ALLOCATOR_H 1

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#include <kernel/fs/file.h>
#include <kernel/mem/vm_region.h>

struct process;

void init_vm_allocator(uintptr_t initrd_phys_start, uintptr_t initrd_phys_end);

void *add_vm_pages_end(size_t n, uint64_t type);
void *add_vm_pages_start(size_t n, uint64_t type);
void remove_vm_pages_end(size_t n, uint64_t type);
void remove_vm_pages_start(size_t n, uint64_t type);

int unmap_range(uintptr_t addr, size_t length);
struct vm_region *map_region(void *addr, size_t len, int prot, uint64_t type);
int map_range_protections(uintptr_t addr, size_t len, int prot);

struct vm_region *find_first_kernel_vm_region();
struct vm_region *find_vm_region(uint64_t type);
struct vm_region *find_user_vm_region_by_addr(uintptr_t addr);
struct vm_region *find_user_vm_region_in_range(uintptr_t start, uintptr_t end);
struct vm_region *clone_process_vm();

void *create_phys_addr_mapping(uintptr_t phys_addr);
void *create_phys_addr_mapping_from_virt_addr(void *virt_addr);

size_t vm_compute_total_virtual_memory(struct process *process);

void dump_kernel_regions(uintptr_t addr);
void dump_process_regions(struct process *process);

struct vm_region *find_kernel_vm_region_by_addr(uintptr_t addr);
bool vm_is_kernel_address(uintptr_t addr);

struct vm_region *vm_allocate_kernel_region(size_t size);
void vm_free_kernel_region(struct vm_region *region);

#endif /* _KERNEL_MEM_VM_ALLOCATOR_H */
