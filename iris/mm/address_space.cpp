#include <iris/mm/address_space.h>
#include <iris/mm/page_frame_allocator.h>
#include <iris/mm/sections.h>

namespace iris::mm {
static auto const heap_start = iris::mm::kernel_end + 4096;

Expected<VirtualAddress> AddressSpace::allocate_region(size_t page_aligned_length) {
    // Basic hack algorithm: allocate the new region at a large fixed offset from the old region.
    // Additionally, immediately fill in the newly created pages.

    auto last_virtual_address = m_regions.back().transform(&Region::end).value_or(heap_start);
    auto new_virtual_address = last_virtual_address + 8192 * 0x1000;

    // TODO: provide the flags as a parameter.
    auto [new_region, did_insert] =
        m_regions.emplace(new_virtual_address, page_aligned_length, RegionFlags::Readable | RegionFlags::Writable | RegionFlags::User);

    for (auto virtual_address : (*new_region).each_page()) {
        TRY(map_physical_page(virtual_address, TRY(allocate_page_frame())));
    }

    return (*new_region).base();
}
}