#ifndef _KERNEL_HAL_PROCESSOR_H
#define _KERNEL_HAL_PROCESSOR_H 1

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <kernel/hal/arch.h>
#include <kernel/util/spinlock.h>

#include HAL_ARCH_SPECIFIC(processor.h)

struct task;
struct vm_region;

enum processor_ipi_message_type { PROCESSOR_IPI_FREED, PROCESSOR_IPI_FLUSH_TLB };

struct processor_ipi_message {
    struct processor_ipi_message *next;
    int ref_count;

    enum processor_ipi_message_type type;
    union {
        struct {
            uintptr_t base;
            size_t pages;
        } flush_tlb;
    };
};
struct processor {
    struct processor *self;
    struct processor *next;

    struct task *idle_task;
    struct vm_region *kernel_stack;

    struct processor_ipi_message *ipi_messages_head;
    struct processor_ipi_message *ipi_messages_tail;
    spinlock_t ipi_messages_lock;

    int id;
    bool enabled;

    struct arch_processor arch_processor;
};

struct processor *create_processor();
struct processor *get_processor_list(void);
struct processor *get_bsp(void);
void add_processor(struct processor *processor);
int processor_count(void);

void arch_init_processor(struct processor *processor);
void init_bsp(struct processor *processor);

void broadcast_panic(void);
void arch_broadcast_panic(void);
void arch_broadcast_ipi(void);
void broadcast_flush_tlb(uintptr_t base, size_t pages);

void init_processor_ipi_messages(void);
struct processor_ipi_message *allocate_processor_ipi_message(void);
void bump_processor_ipi_message(struct processor_ipi_message *message);
void drop_processor_ipi_message(struct processor_ipi_message *message);
void enqueue_processor_ipi_message(struct processor *processor, struct processor_ipi_message *message);
void handle_processor_messages(void);

static inline struct task *get_idle_task(void) {
    return get_current_processor()->idle_task;
}

#endif /* _KERNEL_HAL_HAL_H */
