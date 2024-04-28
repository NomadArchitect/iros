#include <di/platform/custom.h>
#include <diusaudio/frame.h>
#include <diusaudio/frame_info.h>
#include <diusaudio/sink.h>

#ifdef DIUSAUDIO_HAVE_PIPEWIRE
#include "linux/pipewire.h"
#endif

namespace audio {
auto make_sink([[maybe_unused]] SinkCallback callback, [[maybe_unused]] FrameInfo info) -> di::Result<Sink> {
#ifdef DIUSAUDIO_HAVE_PIPEWIRE
    return linux::make_pipewire_sink(di::move(callback), info);
#else
    return di::Unexpected(di::BasicError::OperationNotSupported);
#endif
}
}
