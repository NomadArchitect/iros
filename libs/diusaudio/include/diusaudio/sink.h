#pragma once

#include <di/any/container/any.h>
#include <di/any/dispatch/dispatcher.h>
#include <di/any/types/this.h>
#include <di/function/container/function.h>
#include <di/meta/core.h>
#include <di/types/integers.h>
#include <di/vocab/error/result.h>
#include <diusaudio/frame.h>

namespace audio {
namespace sink {
    struct Start : di::Dispatcher<Start, void(di::This&)> {};
    struct Stop : di::Dispatcher<Stop, void(di::This&)> {};

    using SinkInterface = di::meta::List<Start, Stop>;
}

using SinkCallback = di::Function<void(Frame)>;
using Sink = di::Any<sink::SinkInterface>;

constexpr inline auto start = sink::Start {};
constexpr inline auto stop = sink::Stop {};

auto make_sink(SinkCallback callback, u32 channel_count = 1, SampleFormat format = SampleFormat::Float32LE,
               u32 sample_rate = 44100) -> di::Result<Sink>;
}
