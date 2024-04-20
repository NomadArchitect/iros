#pragma once

#include <di/assert/assert_bool.h>
#include <di/types/byte.h>
#include <di/types/integers.h>
#include <di/vocab/md/extents.h>
#include <di/vocab/md/extents_forward_declaration.h>
#include <di/vocab/md/mdspan.h>
#include <di/vocab/span/prelude.h>
#include <di/vocab/span/span_forward_declaration.h>

namespace gfx {
struct ARGBPixel {
    u8 blue;
    u8 green;
    u8 red;
    u8 alpha;
};

class BitMap {
public:
    BitMap() = default;

    constexpr explicit BitMap(di::Span<byte> data, usize width, usize height)
        : m_data(data), m_width(width), m_height(height) {
        DI_ASSERT(data.size() == width * height * sizeof(ARGBPixel));
    }

    constexpr auto width() const -> usize { return m_width; }
    constexpr auto height() const -> usize { return m_height; }

    auto argb_pixels() const { return di::MDSpan { reinterpret_cast<ARGBPixel*>(m_data.data()), m_height, m_width }; }

private:
    di::Span<byte> m_data;
    usize m_width;
    usize m_height;
};
}
