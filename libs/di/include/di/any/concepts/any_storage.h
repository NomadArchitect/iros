#pragma once

#include <di/any/storage/storage_category.h>
#include <di/concepts/default_constructible.h>
#include <di/concepts/same_as.h>
#include <di/meta/bool_constant.h>
#include <di/types/prelude.h>

namespace di::concepts {
template<typename T>
concept AnyStorage =
    DefaultConstructible<T> && requires {
                                   typename T::Interface;

                                   // Must be a constant expression.
                                   { T::storage_category() } -> SameAs<any::StorageCategory>;
                                   typename types::Nontype<T::storage_category()>;

                                   // This must be evaluatable for any types, not just Void.
                                   { T::creation_is_fallible(in_place_type<Void>) } -> SameAs<bool>;

                                   // Must be a constant expression.
                                   typename meta::BoolConstant<T::creation_is_fallible(in_place_type<Void>)>;
                               };
}