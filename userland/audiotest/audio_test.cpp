#include <di/cli/parser.h>
#include <di/container/algorithm/copy.h>
#include <di/container/algorithm/fill.h>
#include <di/container/algorithm/fill_n.h>
#include <di/container/algorithm/min.h>
#include <di/container/path/path_view.h>
#include <di/container/view/range.h>
#include <di/function/monad/monad_try.h>
#include <di/math/constants.h>
#include <di/math/functions.h>
#include <di/vocab/optional/optional_forward_declaration.h>
#include <dius/main.h>
#include <dius/print.h>
#include <dius/system/process.h>
#include <diusaudio/formats/wav.h>
#include <diusaudio/frame.h>
#include <diusaudio/sink.h>

namespace audiotest {
struct Args {
    di::Optional<di::PathView> wav_file;

    constexpr static auto get_cli_parser() {
        return di::cli_parser<Args>("audio_test"_sv, "Iros audio test program"_sv)
            .flag<&Args::wav_file>('w', "wave"_tsv, "Wave file to play"_sv);
    }
};

di::Result<void> main(Args& args) {
    if (args.wav_file) {
        auto result = TRY(audio::formats::parse_wav(*args.wav_file));

        auto bytes_index = 0;
        auto sink = TRY(audio::make_sink(
            [&](audio::Frame frame) {
                auto output = frame.as_raw_bytes();
                auto to_copy = di::min(output.size(), result.frame.as_raw_bytes().size() - bytes_index);
                di::copy(*result.frame.as_raw_bytes().subspan(bytes_index, to_copy), output.data());
                bytes_index += to_copy;

                if (to_copy == 0) {
                    dius::system::exit_process(0);
                }
            },
            result.frame.channel_count(), result.frame.format(), result.frame.sample_rate_hz()));

        audio::start(sink);

        return {};
    }

    auto sink = TRY(audio::make_sink([](audio::Frame frame) {
        static int f = 220;
        static f32 accumulator = 0;

        auto* out = frame.as_float32_le().data();
        for (auto i : di::range(frame.sample_count())) {
            (void) i;

            accumulator += 2 * di::numbers::pi * f / frame.sample_rate_hz();
            if (accumulator >= 2 * di::numbers::pi) {
                accumulator -= 2 * di::numbers::pi;
            }

            auto val = di::sin(accumulator) * 0.5;
            out = di::fill_n(out, frame.channel_count(), val);
        }
        f++;
        if (f == 880) {
            f = 220;
        }
    }));

    audio::start(sink);

    return {};
}
}

DIUS_MAIN(audiotest::Args, audiotest)
