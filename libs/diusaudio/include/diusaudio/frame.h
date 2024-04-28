#pragma once

#include <di/assert/prelude.h>
#include <di/types/byte.h>
#include <di/types/floats.h>
#include <di/types/integers.h>
#include <di/util/unreachable.h>
#include <di/vocab/span/prelude.h>
#include <diusaudio/frame_info.h>

namespace audio {
class Frame {
public:
    Frame() = default;

    constexpr explicit Frame(di::Span<byte> data, FrameInfo info) : m_data(data), m_info(info) {}

    constexpr auto format() const -> SampleFormat { return m_info.format; }
    constexpr auto channel_count() const -> usize { return m_info.channel_count; }
    constexpr auto sample_rate_hz() const -> usize { return m_info.sample_rate_hz; }
    constexpr auto info() const -> FrameInfo { return m_info; }

    constexpr auto sample_count() const -> usize { return m_data.size() / stride(); }
    constexpr auto bytes_per_sample() const -> usize { return format_bytes_per_sample(format()); }
    constexpr auto stride() const -> usize { return bytes_per_sample() * channel_count(); }

    auto as_float32_le() const -> di::Span<f32> {
        ASSERT_EQ(format(), SampleFormat::Float32LE);
        auto* data = reinterpret_cast<f32*>(m_data.data());
        return { data, sample_count() * channel_count() };
    }

    auto as_signed_int16_le() const -> di::Span<i16> {
        ASSERT_EQ(format(), SampleFormat::SignedInt16LE);
        auto* data = reinterpret_cast<i16*>(m_data.data());
        return { data, sample_count() * channel_count() };
    }

    auto as_signed_int32_le() const -> di::Span<i32> {
        ASSERT_EQ(format(), SampleFormat::SignedInt32LE);
        auto* data = reinterpret_cast<i32*>(m_data.data());
        return { data, sample_count() * channel_count() };
    }

    auto as_raw_bytes() const -> di::Span<byte> { return m_data; }

private:
    di::Span<byte> m_data;
    FrameInfo m_info;
};
}
