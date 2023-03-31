#pragma once

#include <di/prelude.h>
#include <dius/error.h>
#include <dius/sync_file.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

namespace ccpp {
template<typename T>
struct MallocAllocator {
public:
    using Value = T;

    di::Expected<di::container::Allocation<T>, dius::PosixCode> allocate(usize count) const {
        auto* data = [&] {
            if constexpr (alignof(T) > 16) {
                return aligned_alloc(alignof(T), count * sizeof(T));
            }
            return malloc(count * sizeof(T));
        }();

        if (!data) {
            return di::Unexpected(dius::PosixError::NotEnoughMemory);
        }
        return di::container::Allocation<T> { static_cast<T*>(data), count };
    }

    void deallocate(T* pointer, usize) { free(pointer); }
};

struct FileDeleter {
    void operator()(FILE* file) const { (void) fclose(file); }
};

using FileHandle = di::Box<FILE, FileDeleter>;

enum class BufferMode {
    NotBuffered = _IONBF,
    LineBuffered = _IOLBF,
    FullBuffered = _IOFBF,
};

enum class Permissions {
    None = 0,
    Readable = 1,
    Writable = 2,
};

DI_DEFINE_ENUM_BITWISE_OPERATIONS(Permissions)

enum class BufferOwnership {
    Owned,
    UserProvided,
};

struct File {
    explicit File() = default;

    inline ~File() {
        if (buffer_ownership == BufferOwnership::Owned && buffer != nullptr) {
            MallocAllocator<byte>().deallocate(buffer, buffer_capacity);
        }
    }

    dius::SyncFile file;
    byte* buffer { nullptr };
    usize buffer_capacity { 0 };
    usize buffer_offset { 0 };
    usize buffer_size { 0 };
    BufferOwnership buffer_ownership { BufferOwnership::Owned };
    BufferMode buffer_mode { BufferMode::FullBuffered };
    Permissions permissions { Permissions::None };
};

#define STDIO_TRY(...)                                           \
    __extension__({                                              \
        auto __result = (__VA_ARGS__);                           \
        if (!__result) {                                         \
            errno = di::to_underlying(__result.error().value()); \
            return EOF;                                          \
        }                                                        \
        di::util::move(__result).__try_did_succeed();            \
    }).__try_move_out()

#define STDIO_TRY_OR_NULL(...)                                   \
    __extension__({                                              \
        auto __result = (__VA_ARGS__);                           \
        if (!__result) {                                         \
            errno = di::to_underlying(__result.error().value()); \
            return nullptr;                                      \
        }                                                        \
        di::util::move(__result).__try_did_succeed();            \
    }).__try_move_out()

}

struct __file_implementation {
    /// @brief Get the file implementation.
    ///
    /// @warning This can only be called by the 'unlocked' stdio variants.
    ccpp::File& get_unlocked() { return locked.get_assuming_no_concurrent_accesses(); }

    di::Synchronized<ccpp::File> locked;
};
