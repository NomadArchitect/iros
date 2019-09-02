#ifndef _KERNEL_ARCH_X86_64_SCHED_PROCESS_SCHED_H
#define _KERNEL_ARCH_X86_64_SCHED_PROCESS_SCHED_H 1

#include <kernel/sched/process_sched.h>
#include <kernel/proc/process.h>
#include <kernel/arch/x86_64/proc/process.h>

void sched_run_next_entry();
void arch_sched_run_next(struct process_state *process_state);

#endif /* _KERNEL_ARCH_X86_64_SCHED_PROCESS_SCHED_H */