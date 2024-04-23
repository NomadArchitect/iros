#pragma once

#include <di/container/tree/tree_map.h>
#include <di/function/container/function.h>
#include <di/util/exchange.h>
#include <di/util/immovable.h>
#include <di/util/noncopyable.h>
#include <di/vocab/error/result.h>
#include <diusaudio/frame.h>
#include <diusaudio/sink.h>
#include <pipewire/loop.h>
#include <pipewire/main-loop.h>
#include <pipewire/pipewire.h>
#include <pipewire/stream.h>

namespace audio::linux {
class PipewireLibrary : di::NonCopyable {
public:
    PipewireLibrary();
    PipewireLibrary(PipewireLibrary&& other) : m_active(di::exchange(other.m_active, false)) {}
    ~PipewireLibrary();

private:
    bool m_active { false };
};

class PipewireMainloop : di::Immovable {
public:
    PipewireMainloop();
    ~PipewireMainloop();

    void run();
    void quit();
    void register_signal_handler(u32 signo, di::Function<void()>);

    auto raw_loop() const -> pw_loop*;

private:
    pw_main_loop* m_loop { nullptr };
    di::TreeMap<u32, di::Function<void()>> m_handlers;
};

class PipewireStream : di::Immovable {
public:
    explicit PipewireStream(PipewireMainloop& loop, SinkCallback callback, u32 channel_count, SampleFormat format,
                            u32 sample_rate);
    ~PipewireStream();

    void connect();

private:
    pw_stream* m_stream { nullptr };
    SinkCallback m_sink_callback;

    u32 m_channel_count { 1 };
    SampleFormat m_format { SampleFormat::Float32LE };
    u32 m_sample_rate { 44100 };
};

auto make_pipewire_sink(SinkCallback callback, u32 channel_count, SampleFormat format,
                        u32 sample_rate) -> di::Result<Sink>;
}
