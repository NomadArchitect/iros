#pragma once

#include <di/meta/algorithm.h>
#include <di/meta/core.h>
#include <di/meta/function.h>
#include <di/meta/language.h>
#include <di/meta/list.h>
#include <di/meta/operations.h>
#include <di/meta/util.h>
#include <di/types/in_place.h>
#include <di/util/get.h>
#include <di/vocab/tuple/tuple.h>

namespace di::util {
/// @brief A helper class to simulate a single named argument.
///
/// @brief Tag The tag type for the named argument.
/// @brief T The type of the named argument.
///
/// This class models a single named argument, where the name is essentially the tag type. Typically, this class is used
/// with CRTP, so the tag type is the derived class.
///
/// @see NamedArguments
template<typename Tag, typename T>
class NamedArgument {
public:
    using Type = T;

    constexpr static bool is_named_argument = true;

    constexpr explicit NamedArgument()
    requires(concepts::DefaultConstructible<T>)
    = default;

    template<typename U>
    requires(!concepts::DecaySameAs<NamedArgument, U> && concepts::ConstructibleFrom<T, U>)
    // NOLINTNEXTLINE(bugprone-forwarding-reference-overload)
    constexpr explicit(!concepts::ConvertibleTo<U, T>) NamedArgument(U&& value) : m_value(di::forward<U>(value)) {}

    template<typename... Args>
    requires(concepts::ConstructibleFrom<T, Args...>)
    constexpr explicit NamedArgument(InPlace, Args&&... args) : m_value(di::forward<Args>(args)...) {}

    constexpr auto value() & -> T& { return m_value; }
    constexpr auto value() const& -> T const& { return m_value; }
    constexpr auto value() && -> T&& { return di::move(m_value); }
    constexpr auto value() const&& -> T const&& { return di::move(m_value); }

private:
    T m_value;
};

/// @brief A helper class for simulation named arguments in c++.
///
/// @tparam Args The types of the named arguments.
///
/// This class is used to take a list of named arguments and store them in a form easily accessible by the user. This
/// facility includes the helper functions util::get_named_argument and util::get_named_argument_or which can be used to
/// access the named arguments.
///
/// Named arguments are declared using CRTP with the util::NamedArgument class. The current implementation allows all
/// named arguments to be optional, but this may change in the future.
///
/// The folllowing example shows how to use this class and related methods:
/// @snippet{trimleft} tests/test_util.cpp named_arguments
///
/// @see NamedArgument
/// @see get_named_argument
/// @see get_named_argument_or
template<typename... Args>
requires((Args::is_named_argument && ...) && meta::Size<meta::Unique<meta::List<Args && ...>>> == sizeof...(Args))
class NamedArguments {
public:
    constexpr explicit NamedArguments(Args&&... args) : m_arguments(di::forward<Args>(args)...) {}

    using Type = meta::List<Args&&...>;

    constexpr auto arguments() & -> Tuple<Args&&...>& { return m_arguments; }
    constexpr auto arguments() const& -> Tuple<Args&&...> const& { return m_arguments; }
    constexpr auto arguments() && -> Tuple<Args&&...>&& { return di::move(m_arguments); }
    constexpr auto arguments() const&& -> Tuple<Args&&...> const&& { return di::move(m_arguments); }

private:
    Tuple<Args&&...> m_arguments;
};

template<typename... Args>
NamedArguments(Args&&...) -> NamedArguments<Args&&...>;

namespace detail {
    template<typename Arg>
    struct GetNamedArgumentFunction {
        template<concepts::RemoveCVRefInstanceOf<NamedArguments> Args>
        requires(meta::Contains<meta::Type<meta::RemoveCVRef<Args>>, Arg &&>)
        constexpr auto operator()(Args&& args) const -> decltype(auto) {
            return di::get<Arg&&>(di::forward<Args>(args).arguments()).value();
        }
    };
}

/// @brief A helper function to access a named argument.
///
/// @tparam Arg The type of the named argument.
///
/// @param args The named arguments to access.
///
/// @return The value of the named argument.
///
/// This function is used to access a named argument from a list of named arguments. Note that it is a compile-time
/// error to call this function with a named argument that is not present in the list of named arguments. Therefore, the
/// concepts::HasNamedArgument concept should be used with an `if constexpr` block before calling this method.
/// Otherwise, use the util::get_named_argument_or function.
///
/// @note This function propogates the value-category of the passed named argument pack, which means that the named
/// argument pack should be passed as an rvalue reference (unless of course, the named argument needs to be read
/// multiple times).
///
/// @see NamedArguments
/// @see get_named_argument_or
template<typename Arg>
constexpr inline auto get_named_argument = detail::GetNamedArgumentFunction<Arg> {};

namespace detail {
    template<typename Arg>
    struct GetNamedArgumentOrFunction {
        template<concepts::RemoveCVRefInstanceOf<NamedArguments> Args, typename Val = meta::Type<Arg>,
                 concepts::ConvertibleTo<Val> U>
        requires(concepts::ConstructibleFrom<Val, meta::Like<Args, Val>>)
        constexpr auto operator()(Args&& args, U&& fallback) const -> Val {
            if constexpr (meta::Contains<meta::Type<meta::RemoveCVRef<Args>>, Arg&&>) {
                return get_named_argument<Arg>(args);
            } else {
                return di::forward<U>(fallback);
            }
        }
    };
}

/// @brief A helper function to access a named argument or a fallback value.
///
/// @tparam Arg The type of the named argument.
///
/// @param args The named arguments to access.
/// @param fallback The fallback value to use if the named argument is not present.
///
/// @return The value of the named argument or the fallback value.
///
/// This function is used to access a named argument from a list of named arguments. If the named argument is not
/// present in the list of named arguments, the fallback value is returned instead. To prevent dangling references, the
/// returned argument is decay copied out.
///
/// @note This function propogates the value-category of the passed named argument pack, which means that, to avoid
/// copies, the named should be passed as an rvalue reference (unless of course, the named argument needs to be read
/// multiple times).
///
/// @see NamedArguments
template<typename Arg>
constexpr inline auto get_named_argument_or = detail::GetNamedArgumentOrFunction<Arg> {};
}

namespace di::concepts {
/// @brief A concept to check if a named argument is present.
///
/// @tparam Args The type of the named arguments.
/// @tparam Arg The type of the named argument.
///
/// This concept checks if a named argument is present in a list of named arguments.
///
/// @see NamedArguments
template<typename Args, typename Arg>
concept HasNamedArgument = concepts::InstanceOf<Args, util::NamedArguments> && meta::Contains<meta::Type<Args>, Arg&&>;

/// @brief A concept to check if a list of named arguments is valid.
/// @tparam Allowed The list of allowed named arguments.
/// @tparam Args The list of arguments passed to the function.
///
/// This concept checks if a list of named arguments is valid. A list of named arguments is valid if it contains only
/// named arguments from the list of allowed named arguments. Note that for now, this concept assumes all named
/// arguments are optional.
///
/// @see NamedArguments
template<typename Allowed, typename... Args>
concept ValidNamedArguments =
    meta::Size<meta::Unique<
        meta::Concat<meta::Transform<Allowed, meta::Quote<meta::AddRValueReference>>, meta::List<Args&&...>>>> ==
    meta::Size<Allowed>;
}

namespace di {
using concepts::HasNamedArgument;
using concepts::ValidNamedArguments;
using util::get_named_argument;
using util::get_named_argument_or;
using util::NamedArgument;
using util::NamedArguments;
}
