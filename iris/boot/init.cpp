#include <di/math/prelude.h>
#include <di/meta/constexpr.h>
#include <iris/arch/x86/amd64/hw/serial.h>
#include <iris/boot/cxx_init.h>
#include <iris/boot/init.h>
#include <iris/core/global_state.h>
#include <iris/core/interruptible_spinlock.h>
#include <iris/core/print.h>
#include <iris/core/scheduler.h>
#include <iris/core/task.h>
#include <iris/fs/debug_file.h>
#include <iris/fs/initrd.h>
#include <iris/fs/tmpfs.h>
#include <iris/hw/acpi/acpi.h>
#include <iris/mm/address_space.h>
#include <iris/mm/map_physical_address.h>
#include <iris/mm/page_frame_allocator.h>
#include <iris/mm/physical_address.h>
#include <iris/mm/sections.h>
#include <iris/third_party/limine.h>

static void do_unit_tests() {
    iris::test::TestManager::the().run_tests();
    iris::current_scheduler()->exit_current_task();
}

extern "C" {
static limine_memmap_request volatile memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0,
    .response = nullptr,
};

static limine_kernel_address_request volatile kernel_address_request = {
    .id = LIMINE_KERNEL_ADDRESS_REQUEST,
    .revision = 0,
    .response = nullptr,
};

static limine_module_request volatile module_request = {
    .id = LIMINE_MODULE_REQUEST,
    .revision = 0,
    .response = nullptr,
    .internal_module_count = 0,
    .internal_modules = nullptr,
};

static limine_kernel_file_request volatile kernel_file_request = {
    .id = LIMINE_KERNEL_FILE_REQUEST,
    .revision = 0,
    .response = nullptr,
};
}

namespace iris {
static auto kernel_command_line = di::container::string::StringImpl<di::container::string::TransparentEncoding,
                                                                    di::StaticVector<char, di::Constexpr<4096zu>>> {};

void iris_main() {
    iris::println("Starting architecture independent initialization..."_sv);

    if (!kernel_file_request.response) {
        println("No limine kernel file response, panicking..."_sv);
        ASSERT(false);
    }

    auto bootloader_command_line = di::ZString(kernel_file_request.response->kernel_file->cmdline);
    auto command_line_size = di::distance(bootloader_command_line);
    if (command_line_size > 4096) {
        println("Kernel command line is too large, panicking..."_sv);
        ASSERT(false);
    }

    for (auto c : bootloader_command_line) {
        (void) kernel_command_line.push_back(c);
    }

    println("Kernel command line: {:?}"_sv, kernel_command_line);

    auto& global_state = global_state_in_boot();
    global_state.heap_start = iris::mm::VirtualAddress(di::align_up(iris::mm::kernel_end.raw_value(), 4096));
    global_state.heap_end = global_state.heap_start;

    auto memory_map = di::Span { memmap_request.response->entries, memmap_request.response->entry_count };

    ASSERT(!memory_map.empty());
    global_state.max_physical_address = di::max(memory_map | di::filter([&](auto* entry) {
                                                    // Ignore anything over 4 GiB for now.
                                                    return mm::PhysicalAddress(entry->base) + entry->length <
                                                           mm::PhysicalAddress(4_u64 * 1024_u64 * 1024_u64 * 1024_u64);
                                                }) |
                                                di::transform([&](auto* entry) {
                                                    return mm::PhysicalAddress(entry->base) + entry->length;
                                                }));

    for (auto* memory_map_entry : memory_map) {
        iris::println("Memory map entry: type: {}, base: {:#018x}, length: {:#018x}"_sv, memory_map_entry->type,
                      memory_map_entry->base, memory_map_entry->length);
        if (memory_map_entry->type != LIMINE_MEMMAP_USABLE) {
            iris::mm::reserve_page_frames(iris::mm::PhysicalAddress(di::align_down(memory_map_entry->base, 4096)),
                                          di::divide_round_up(memory_map_entry->length, 4096));
        } else {
            iris::mm::unreserve_page_frames(iris::mm::PhysicalAddress(di::align_down(memory_map_entry->base, 4096)),
                                            di::divide_round_up(memory_map_entry->length, 4096));
        }
    }

    iris::mm::reserve_page_frames(iris::mm::PhysicalAddress(0), 64);

    ASSERT_GT(module_request.response->module_count, 0u);
    auto initrd_module = *module_request.response->modules[0];

    iris::println("Kernel virtual base: {:#018x}"_sv, kernel_address_request.response->virtual_base);
    iris::println("Kernel physical base: {:#018x}"_sv, kernel_address_request.response->physical_base);
    iris::println("Max physical memory: {}"_sv, global_state.max_physical_address);
    iris::println("Module base address: {}"_sv, initrd_module.address);

    // NOTE: the limine boot loader places the module in the HHDM, and marks the region and kernel+modules,
    //       so we can safely access its provided virtual address after unloading the boot loader. This relies
    //       on limine placing its HHDM at the same location we do, which is unsafe but works for now.
    global_state.initrd = di::Span { static_cast<di::Byte const*>(initrd_module.address), initrd_module.size };

    ASSERT(mm::init_and_load_initial_kernel_address_space(
        mm::PhysicalAddress(kernel_address_request.response->physical_base),
        mm::VirtualAddress(kernel_address_request.response->virtual_base), global_state.max_physical_address));

    // Perform architecture specific initialization which requires allocation now that the address space is set up.
    iris::println("Starting final architecture specific initialization..."_sv);
    arch::init_final();

    // Setup initrd.
    ASSERT(iris::init_initrd());

    // Setup temp file system.
    ASSERT(iris::init_tmpfs());

    auto init_task = *iris::create_kernel_task(global_state.task_namespace, [] {
        println("Running kernel init task..."_sv);
        arch::init_task();

        auto& global_state = global_state_in_boot();
        {
            ASSERT(!interrupts_disabled());
            auto task_finalizer = *iris::create_kernel_task(global_state.task_namespace, [] {
                for (;;) {
                    auto record = di::Optional<TaskFinalizationRequest> {};
                    *iris::global_state().task_finalization_wait_queue.wait([&] {
                        record = iris::global_state().task_finalization_data_queue.pop();
                        return record.has_value();
                    });

                    (void) iris::global_state().kernel_address_space.lock()->destroy_region(record->kernel_stack,
                                                                                            0x2000);
                }
            });
            schedule_task(*task_finalizer);

            *global_state.initial_fpu_state.setup_initial_fpu_state();

            auto init_path = kernel_command_line.empty() ? "/sh"_pv : di::PathView(kernel_command_line);
            if (init_path.data() == "-run=kernel_unit_test"_tsv) {
                iris::println("Preparing to run kernel unit tests."_sv);

                auto test_runner = *iris::create_kernel_task(global_state.task_namespace, do_unit_tests);
                schedule_task(*test_runner);
            } else {
                iris::println("Loading initial userspace task: {}"_sv, init_path);

                auto file_table = iris::FileTable {};
                auto debug_file = *iris::File::create(di::in_place_type<DebugFile>);
                di::get<0>(*file_table.allocate_file_handle()) = debug_file;
                di::get<0>(*file_table.allocate_file_handle()) = debug_file;
                di::get<0>(*file_table.allocate_file_handle()) = debug_file;

                auto task4 = *iris::create_user_task(global_state.task_namespace, global_state.initrd_root,
                                                     global_state.initrd_root, di::move(file_table), nullptr);

                auto arguments = di::Vector<di::TransparentString> {};
                *arguments.push_back(*init_path.data().to_owned());

                auto task_arguments =
                    *di::make_arc<TaskArguments>(di::move(arguments), di::Vector<di::TransparentString> {});
                task4->set_task_arguments(di::move(task_arguments));

                *iris::load_executable(*task4, init_path);

                schedule_task(*task4);
            }
        }

        println("Finished kernel init task..."_sv);
        iris::current_scheduler()->exit_current_task();
    });

    iris::println("Starting the kernel scheduler..."_sv);

    auto& scheduler = global_state.boot_processor.scheduler();
    scheduler.schedule_task(*init_task);
    scheduler.start();
}
}
