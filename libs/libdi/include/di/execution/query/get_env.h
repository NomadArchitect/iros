#pragma once

#include <di/concepts/not_same_as.h>
#include <di/execution/types/no_env.h>
#include <di/function/tag_invoke.h>

namespace di::execution {
namespace detail {
    struct GetEnvFunction {
        template<typename T>
        requires(concepts::TagInvocable<GetEnvFunction, T const&>)
        constexpr /* concepts::NotSameAs<types::NoEnv> */ auto operator()(T const& value) const {
            return function::tag_invoke(*this, value);
        }

        // FIXME: is this really necessary?
        constexpr types::NoEnv operator()(auto const&) const { return {}; }
    };
}

constexpr inline auto get_env = detail::GetEnvFunction {};
}