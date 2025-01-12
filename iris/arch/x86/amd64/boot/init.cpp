#include <iris/arch/x86/amd64/gdt.h>
#include <iris/arch/x86/amd64/hw/io_apic.h>
#include <iris/arch/x86/amd64/hw/local_apic.h>
#include <iris/arch/x86/amd64/hw/pic.h>
#include <iris/arch/x86/amd64/hw/pit.h>
#include <iris/arch/x86/amd64/hw/sb16.h>
#include <iris/arch/x86/amd64/hw/serial.h>
#include <iris/arch/x86/amd64/idt.h>
#include <iris/arch/x86/amd64/io_instructions.h>
#include <iris/arch/x86/amd64/msr.h>
#include <iris/arch/x86/amd64/segment_descriptor.h>
#include <iris/arch/x86/amd64/system_instructions.h>
#include <iris/arch/x86/amd64/system_segment_descriptor.h>
#include <iris/arch/x86/amd64/tss.h>
#include <iris/boot/cxx_init.h>
#include <iris/boot/init.h>
#include <iris/core/global_state.h>
#include <iris/core/interrupt_disabler.h>
#include <iris/core/print.h>
#include <iris/core/scheduler.h>
#include <iris/core/task.h>
#include <iris/core/wait_queue.h>
#include <iris/fs/debug_file.h>
#include <iris/fs/file.h>
#include <iris/fs/initrd.h>
#include <iris/hw/acpi/acpi.h>
#include <iris/hw/power.h>
#include <iris/hw/timer.h>
#include <iris/mm/address_space.h>
#include <iris/mm/map_physical_address.h>
#include <iris/mm/page_frame_allocator.h>
#include <iris/mm/sections.h>
#include <iris/uapi/syscall.h>

namespace iris::arch {
alignas(4096) static di::Array<byte, 16384> bsp_stack;

extern "C" void bsp_cpu_init() {
    iris::arch::cxx_init();

    iris::x86::amd64::init_serial_early_boot();

    auto& global_state = global_state_in_boot();
    global_state.boot_processor.arch_processor().set_fallback_kernel_stack(di::to_uintptr(bsp_stack.data()));

    iris::println("Beginning x86_64 kernel boot..."_sv);

    global_state.processor_info = detect_processor_info();
    global_state.processor_info.print_to_console();

    global_state.boot_processor.arch_processor().enable_cpu_features();

    set_current_processor(global_state.boot_processor);
    global_state.current_processor_available = true;

    x86::amd64::idt::init_idt();

    x86::amd64::init_tss();
    x86::amd64::init_gdt();

    set_current_processor(global_state.boot_processor);

    // Setup the page fault handler.
    *register_exception_handler(GlobalIrqNumber(14), [](IrqContext& context) -> IrqStatus {
        auto instruction_pointer = mm::VirtualAddress(context.task_state.rip);
        auto address = mm::VirtualAddress(x86::amd64::read_cr2());

        if (instruction_pointer == kernel_userspace_copy_instruction) {
            context.task_state.set_instruction_pointer(kernel_userspace_copy_return.raw_value());
            context.task_state.rax = di::to_underlying(Error::BadAddress);
            return IrqStatus::Handled;
        }

        println("ERROR: Unexpected page fault: ip={}, address={}"_sv, instruction_pointer, address);
        ASSERT(false);
        return IrqStatus::Handled;
    });

    iris_main();
}

void init_final() {
    acpi::init_acpi();

    auto& global_state = global_state_in_boot();
    if (global_state.acpi_info) {
        global_state.arch_readonly_state.use_apic = true;
    }

    iris::x86::amd64::init_local_apic();
    iris::x86::amd64::init_io_apic();
    iris::x86::amd64::init_pic();

    iris::x86::amd64::init_pit();

    iris::init_timer_assignments();
}

void init_task() {
    iris::x86::amd64::init_alternative_processors();

    iris::x86::amd64::init_serial();
    iris::x86::amd64::init_sb16();
}

extern "C" [[gnu::naked]] void iris_entry() {
    asm volatile("mov %0, %%rsp\n"
                 "push $0\n"
                 "call bsp_cpu_init\n"
                 :
                 : "r"(bsp_stack.data() + bsp_stack.size())
                 : "memory");
}
}
