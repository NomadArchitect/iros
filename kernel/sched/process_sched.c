#include <stddef.h>

#include <kernel/hal/output.h>

#include <kernel/proc/process.h>
#include <kernel/sched/process_sched.h>
#include <kernel/util/spinlock.h>

static struct process *list_start = NULL;
static struct process *list_end = NULL;
static spinlock_t process_list_lock = SPINLOCK_INITIALIZER;

void init_process_sched() {
    arch_init_process_sched();
}

void sched_add_process(struct process *process) {
    spin_lock(&process_list_lock);

    if (list_start == NULL) {
        list_start = list_end = process;
        process->prev = process->next = process;
        
        spin_unlock(&process_list_lock);
        return;
    }

    process->prev = list_end;
    process->next = list_start;

    list_end->next = process;
    list_start->prev = process;
    list_end = process;

    struct process *p = list_start;
    do {
        debug_log("Process: [ %d, %#.16lX, %#.16lX, %#.16lX ]\n", p->pid, (uintptr_t) p, (uintptr_t) p->prev, (uintptr_t) p->next);
        p = p->next;
    } while (p != list_start);

    spin_unlock(&process_list_lock);
}

void sched_remove_process(struct process *process) {
    spin_lock(&process_list_lock);

    if (list_start == NULL) {        
        spin_unlock(&process_list_lock);
        return;
    }

    struct process *p = list_start;
    do {
        debug_log("Process: [ %d, %#.16lX, %#.16lX, %#.16lX ]\n", p->pid, (uintptr_t) p, (uintptr_t) p->prev, (uintptr_t) p->next);
        p = p->next;
    } while (p != list_start);

    struct process *current = list_start;

    while (current->next != process) {
        current = current->next;
    }

    if (process == list_end) {
        list_end = process->prev;
    }

    if (process == list_start) {
        list_start = process->next;
    }

    current->next = current->next->next;
    current->next->prev = current;

    p = list_start;
    do {
        debug_log("Process: [ %d, %#.16lX, %#.16lX, %#.16lX ]\n", p->pid, (uintptr_t) p, (uintptr_t) p->prev, (uintptr_t) p->next);
        p = p->next;
    } while (p != list_start);

    spin_unlock(&process_list_lock);
}

/* Must be called from unpremptable context */
void sched_run_next() {
    struct process *current = get_current_process();

    while (current->next->sched_state != READY) {
        if (current->next->sched_state == EXITING) {
            struct process *to_remove = current->next;

            current->next = current->next->next;
            current->next->prev = current;

            free_process(to_remove);
            continue;
        }

        /* Skip processes that are sleeping */
        current->next = current->next->next;
    }

    run_process(current->next);
}

