#ifndef _KERNEL_MEM_VM_REGION_H
#define _KERNEL_MEM_VM_REGION_H

#include <stdint.h>
#include <stddef.h>

struct vm_region {
    uintptr_t start;
    uintptr_t end;

#define VM_READ (1 << 0)
#define VM_WRITE (1 << 2)
#define VM_USER (1 << 3)
#define VM_NO_EXEC (1 << 4)
    uint64_t flags;

#define VM_KERNEL (1 << 0)
#define VM_VGA (1 << 1)
#define VM_INITRD (1 << 2)
#define VM_KERNEL_HEAP (1 << 3)
    uint64_t type;

    struct vm_region *next;
};

struct vm_region *add_vm_region(struct vm_region *list, struct vm_region *to_add);
struct vm_region *free_vm_region(struct vm_region *list, uintptr_t start);
struct vm_region *get_vm_region(struct vm_region *list, uintptr_t start);
struct vm_region *get_vm_region_by_type(struct vm_region *list, uint64_t type);
int extend_vm_region(struct vm_region *list, uintptr_t start, size_t num_pages);
int contract_vm_region(struct vm_region *list, uintptr_t start, size_t num_pages);

#endif /* _KERNEL_MEM_VM_REGION_H */