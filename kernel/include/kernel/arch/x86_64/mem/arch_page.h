#ifndef _KERNEL_ARCH_X86_64_MEM_ARCH_PAGE_H
#define _KERNEL_ARCH_X86_64_MEM_ARCH_PAGE_H 1

#include <stdint.h>

#include <kernel/mem/vm_region.h>

#define PAGE_SIZE 4096

#define RECURSIVE_PML4_BASE ((uint64_t *) 0xFFFFFFFFFFFFF000)
#define RECURSIVE_PDP_BASE  ((uint64_t *) 0xFFFFFFFFFFE00000)
#define RECURSIVE_PD_BASE   ((uint64_t *) 0xFFFFFFFFC0000000)
#define RECURSIVE_PT_BASE   ((uint64_t *) 0xFFFFFF8000000000)

#define MAX_PML4_ENTRIES (PAGE_SIZE / sizeof(uint64_t))
#define MAX_PDP_ENTRIES  (MAX_PML4_ENTRIES)
#define MAX_PD_ENTRIES   (MAX_PML4_ENTRIES)
#define MAX_PT_ENTRIES   (MAX_PML4_ENTRIES)

#define PML4_RECURSIVE_INDEX (MAX_PML4_ENTRIES - 1)

#define PAGE_STRUCTURE_FLAGS (0x01UL | VM_WRITE)

#define VIRT_ADDR(pml4_offset, pdp_offset, pd_offset, pt_offset)                                                              \
    ((((pml4_offset) &0x100UL) ? 0xFFFFUL << 48 : 0UL) | (((pml4_offset) &0x1FFUL) << 39) | (((pdp_offset) &0x1FFUL) << 30) | \
     (((pd_offset) &0x1FFUL) << 21) | (((pt_offset) &0x1FFUL) << 12))

struct process;

void do_map_phys_page(uintptr_t phys_addr, uintptr_t virt_addr, uint64_t flags, bool broadcast_flush_tlb, struct process *process);
void do_unmap_page(uintptr_t virt_addr, bool free_phys, bool free_phys_structure, bool broadcast_tlb_flush, struct process *process);

void create_phys_id_map();

#endif /* _KERNEL_ARCH_X86_64_MEM_ARCH_PAGE_H */
