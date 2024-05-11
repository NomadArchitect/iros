#pragma once

#include <iris/core/error.h>
#include <iris/core/userspace_buffer.h>

namespace iris::x86::amd64 {
void init_sb16();

iris::Expected<usize> sb16_write_audio(UserspaceBuffer<byte const> data);
}
