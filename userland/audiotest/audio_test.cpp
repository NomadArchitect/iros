#include <di/cli/parser.h>
#include <di/container/algorithm/fill.h>
#include <di/container/algorithm/fill_n.h>
#include <di/container/view/range.h>
#include <di/function/monad/monad_try.h>
#include <dius/main.h>
#include <diusaudio/frame.h>
#include <diusaudio/sink.h>
#include <math.h>

namespace audiotest {
struct Args {
    constexpr static auto get_cli_parser() {
        return di::cli_parser<Args>("audio_test"_sv, "Iros audio test program"_sv);
    }
};

di::Result<void> main(Args&) {
    auto sink = TRY(audio::make_sink([](audio::Frame frame) {
        static int f = 220;
        static f32 accumulator = 0;

        auto* out = frame.as_float32_le().data();
        for (auto i : di::range(frame.sample_count())) {
            (void) i;

            accumulator += 2 * M_PI * f / frame.sample_rate_hz();
            if (accumulator >= 2 * M_PI) {
                accumulator -= 2 * M_PI;
            }

            auto val = sinf(accumulator) * 0.5;
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
