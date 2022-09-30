#pragma once

#include <di/concepts/integer.h>
#include <di/concepts/signed_integer.h>
#include <di/format/formatter.h>
#include <di/math/abs.h>
#include <di/math/to_unsigned.h>

namespace di::format {
template<concepts::Integer T>
constexpr auto tag_invoke(types::Tag<formatter_in_place>, InPlaceType<T>, concepts::FormatParseContext auto&) {
    return [](concepts::FormatContext auto& context, T value) {
        if (value == 0) {
            context.output('0');
            return;
        }

        if constexpr (concepts::SignedInteger<T>) {
            if (value < 0) {
                context.output('-');
            }
        }
        auto as_unsigned = math::to_unsigned(math::abs(value));

        unsigned strength = 1;
        for (auto x = as_unsigned; x / strength >= 10; strength *= 10) {}

        for (; strength; strength /= 10) {
            context.output((as_unsigned / strength) % 10 + '0');
        }
    };
}
}