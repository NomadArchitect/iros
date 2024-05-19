#pragma once

#include <di/cli/prelude.h>
#include <di/container/string/prelude.h>
#include <di/container/vector/prelude.h>
#include <dius/print.h>

#define DIUS_MAIN(Type, NS)                                                                    \
    int main(int argc, char** argv) {                                                          \
        auto args = ::di::Vector<::di::TransparentStringView> {};                              \
        for (int i = 0; i < argc; i++) {                                                       \
            char* arg = argv[i];                                                               \
            size_t len = 0;                                                                    \
            while (arg[len] != '\0') {                                                         \
                len++;                                                                         \
            }                                                                                  \
            args.push_back({ arg, len });                                                      \
        }                                                                                      \
                                                                                               \
        auto as_span = args.span();                                                            \
        auto parser = ::di::get_cli_parser<Type>();                                            \
        auto result = parser.parse(as_span);                                                   \
        if (!result) {                                                                         \
            dius::eprintln("Failed to parse command line arguments."_sv);                      \
            if constexpr (requires { result->help; }) {                                        \
                dius::eprintln("Run with '--help' for all list of valid options."_sv);         \
            }                                                                                  \
            return 2;                                                                          \
        }                                                                                      \
                                                                                               \
        if constexpr (requires { result->help; }) {                                            \
            if (result->help) {                                                                \
                auto fd_writer = ::dius::SyncFile(::dius::SyncFile::Owned::No, 2);             \
                parser.write_help(fd_writer);                                                  \
                return 0;                                                                      \
            }                                                                                  \
        }                                                                                      \
                                                                                               \
        using Result = decltype(NS::main(*result));                                            \
        if constexpr (::di::concepts::Expected<Result>) {                                      \
            auto main_result = NS::main(*result);                                              \
            if (!main_result) {                                                                \
                dius::eprintln("Command failed: error: {}"_sv, main_result.error().message()); \
                return 1;                                                                      \
            }                                                                                  \
        } else {                                                                               \
            (void) NS::main(*result);                                                          \
        }                                                                                      \
        return 0;                                                                              \
    }
