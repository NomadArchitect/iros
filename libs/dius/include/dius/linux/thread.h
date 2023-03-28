#pragma once

#include <dius/error.h>
#include <dius/memory_region.h>
#include <dius/runtime/tls.h>

#ifndef DIUS_USE_RUNTIME
#include <dius/posix/thread.h>
#else
namespace dius {
struct PlatformThread;

struct SelfPointer {
    explicit SelfPointer() : self(static_cast<PlatformThread*>(static_cast<void*>(this))) {}

    PlatformThread* self { nullptr };
};

struct PlatformThread : SelfPointer {
    static di::Result<di::Box<PlatformThread>> create(runtime::TlsInfo);

    PlatformThread() = default;

    int id() const { return thread_id; }
    di::Result<void> join();

    di::Span<byte> thread_local_storage(usize tls_size) {
        return { reinterpret_cast<byte*>(this) - tls_size, tls_size };
    }

    int thread_id { 0 };
    di::Function<void()> entry;
    MemoryRegion stack;
};

di::Result<void> spawn_thread(PlatformThread&);
}
#endif