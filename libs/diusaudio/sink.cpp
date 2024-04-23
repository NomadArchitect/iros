#include <di/platform/custom.h>
#include <diusaudio/frame.h>
#include <diusaudio/sink.h>

#ifdef DIUSAUDIO_HAVE_PIPEWIRE
#include "linux/pipewire.h"
#endif

namespace audio {
auto make_sink([[maybe_unused]] SinkCallback callback, [[maybe_unused]] u32 channel_count,
               [[maybe_unused]] SampleFormat format, [[maybe_unused]] u32 sample_rate) -> di::Result<Sink> {
#ifdef DIUSAUDIO_HAVE_PIPEWIRE
    return linux::make_pipewire_sink(di::move(callback), channel_count, format, sample_rate);
#else
    return di::Unexpected(di::BasicError::OperationNotSupported);
#endif
}
}
