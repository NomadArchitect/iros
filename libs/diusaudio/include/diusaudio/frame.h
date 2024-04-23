#pragma once

#include <di/assert/prelude.h>
#include <di/types/byte.h>
#include <di/types/floats.h>
#include <di/types/integers.h>
#include <di/util/unreachable.h>
#include <di/vocab/span/prelude.h>

namespace audio {
enum class SampleFormat {
    SignedInt16LE,
    SignedInt24LE,
    SignedInt32LE,
    Float32LE,
};

constexpr auto format_bytes_per_sample(SampleFormat format) -> usize {
    using enum SampleFormat;
    switch (format) {
        case SignedInt16LE:
            return 2;
        case SignedInt24LE:
            return 3;
        case SignedInt32LE:
        case Float32LE:
            return 4;
    }
    di::unreachable();
}

class Frame {
public:
    Frame() = default;

    constexpr explicit Frame(di::Span<byte> data, u32 channels, SampleFormat format, u32 sample_rate_hz)
        : m_data(data), m_channel_count(channels), m_format(format), m_sample_rate_hz(sample_rate_hz) {}

    constexpr auto format() const -> SampleFormat { return m_format; }
    constexpr auto channel_count() const -> usize { return m_channel_count; }
    constexpr auto sample_rate_hz() const -> usize { return m_sample_rate_hz; }

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

private:
    di::Span<byte> m_data;
    u32 m_channel_count { 1 };
    SampleFormat m_format { SampleFormat::Float32LE };
    u32 m_sample_rate_hz { 44100 };
};
}
