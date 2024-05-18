#pragma once

namespace dius::net {
enum class Shutdown {
    Read = 0,
    Write = 1,
    ReadWrite = 2,
};
}
