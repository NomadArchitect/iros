#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <kernel/fs/vfs.h>
#include <kernel/irqs/handlers.h>
#include <kernel/mem/page_frame_allocator.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/proc/task.h>
#include <kernel/sched/task_sched.h>
#include <kernel/hal/hal.h>
#include <kernel/hal/output.h>
#include <kernel/net/net.h>

void kernel_main(uintptr_t kernel_phys_start, uintptr_t kernel_phys_end, uintptr_t inintrd_phys_start, uint64_t initrd_phys_end, uint32_t *multiboot_info) {
    init_hal();
    init_irq_handlers();
    init_page_frame_allocator(kernel_phys_start, kernel_phys_end, inintrd_phys_start, initrd_phys_end, multiboot_info);
    init_kernel_task();
    init_vm_allocator(inintrd_phys_start, initrd_phys_end);
    init_vfs();
    init_drivers();
    init_task_sched();
    init_net();

    /* Mount hdd0 at / */
    int error = 0;
    error = fs_mount("/dev/hdd0", "/", "ext2");
    assert(error == 0);

    // Start Shell
    struct task *shell = load_task("/bin/start");
    sched_add_task(shell);

    sched_run_next();

    while (1);
}