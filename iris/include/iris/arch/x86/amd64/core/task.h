#pragma once

#include <di/prelude.h>

#include <iris/core/error.h>

namespace iris::arch {
// This is x86_64 specific.
struct TaskState {
    explicit TaskState(u64 entry, u64 stack, bool userspace);

    /// Function to perform a context switch.
    [[noreturn]] void context_switch_to();

    // These are the main x86_64 registers.
    u64 r15 { 0 };
    u64 r14 { 0 };
    u64 r13 { 0 };
    u64 r12 { 0 };
    u64 r11 { 0 };
    u64 r10 { 0 };
    u64 r9 { 0 };
    u64 r8 { 0 };
    u64 rbp { 0 };
    u64 rdi { 0 };
    u64 rsi { 0 };
    u64 rdx { 0 };
    u64 rcx { 0 };
    u64 rbx { 0 };
    u64 rax { 0 };

    // This is the task state present on the stack after
    // an interrupt occurs.
    u64 rip { 0 };
    u64 cs { 0 };
    u64 rflags { 0 };
    u64 rsp { 0 };
    u64 ss { 0 };
};

struct FpuState {
    FpuState() {}

    ~FpuState();

    /// Setup the task's FPU state. This must be called for userspace tasks.
    Expected<void> setup_fpu_state();

    /// Setup initial FPU state. This creates a clean-copy of the FPU state which is copied to newly created tasks. This
    /// also configures the processor to allow floating-point / SIMD operations.
    Expected<void> setup_initial_fpu_state();

    /// Load this task's FPU state into the registers. This only makes sense to call when IRQs are disabled, right
    /// before performing a context switch.
    void load();

    /// Save the current processor's FPU state into this task. This only makes sense to call when IRQs are disabled,
    /// when the task has just been interrupted.
    void save();

    /// This is the task's FPU state. It is null for kernel-space tasks. Since it is dynamically sized, it must be
    /// managed manually.
    di::Byte* fpu_state { nullptr };

private:
    Expected<di::Byte*> allocate_fpu_state();
};

void load_kernel_stack(mm::VirtualAddress base);
}
