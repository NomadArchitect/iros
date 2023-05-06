#pragma once

#include <di/function/tag_invoke.h>
#include <di/meta/list/as_list.h>
#include <di/meta/remove_cvref.h>
#include <di/reflect/atom.h>
#include <di/types/prelude.h>
#include <di/vocab/tuple/tuple_like.h>

namespace di::concepts {
template<typename T>
concept ReflectionValue = concepts::TupleLike<T> || concepts::InstanceOf<T, reflection::Atom>;
}

namespace di::reflection {
namespace detail {
    struct ReflectFunction {
        template<typename T, typename U = meta::RemoveCVRef<T>>
        requires(concepts::TagInvocable<ReflectFunction, InPlaceType<U>>)
        constexpr decltype(auto) operator()(InPlaceType<T>) const {
            using R = meta::TagInvokeResult<ReflectFunction, InPlaceType<U>>;
            static_assert(concepts::ReflectionValue<R>, "Reflect function must return a tuple of fields or an atom");
            return function::tag_invoke(*this, in_place_type<U>);
        }

        template<typename T, typename U = meta::RemoveCVRef<T>>
        requires(!concepts::TagInvocable<ReflectFunction, InPlaceType<U>> &&
                 (concepts::SameAs<U, bool> || concepts::Integer<U> || concepts::detail::ConstantString<U> ||
                  concepts::Container<U>) )
        constexpr decltype(auto) operator()(InPlaceType<T>) const {
            return Atom<U> {};
        }

        template<typename T, typename U = meta::RemoveCVRef<T>>
        requires(!concepts::InstanceOf<U, InPlaceType> &&
                 requires { (util::declval<ReflectFunction const&>())(in_place_type<U>); })
        constexpr decltype(auto) operator()(T&&) const {
            return (*this)(in_place_type<U>);
        }
    };
}

constexpr inline auto reflect = detail::ReflectFunction {};
}

namespace di::concepts {
template<typename T>
concept Reflectable = requires {
    { reflection::reflect(util::declval<T>()) };
};

template<typename T>
concept ReflectableToAtom = requires {
    { reflection::reflect(util::declval<T>()) } -> InstanceOf<reflection::Atom>;
};

template<typename T>
concept ReflectableToFields = requires {
    { reflection::reflect(util::declval<T>()) } -> TupleLike;
};
}

namespace di::meta {
template<concepts::Reflectable T, typename R = decltype(reflection::reflect(in_place_type<T>))>
using Reflect = meta::Conditional<concepts::ReflectableToFields<T>, meta::AsList<R>, R>;
}
