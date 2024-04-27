#pragma once

#include <di/container/path/path_view.h>
#include <di/container/vector/vector.h>
#include <di/types/byte.h>
#include <di/vocab/pointer/box.h>
#include <diusaudio/frame.h>

namespace audio::formats {
struct WavResult {
    di::Box<di::Vector<byte>> data;
    Frame frame;
};

auto parse_wav(di::PathView path) -> di::Result<WavResult>;
}
