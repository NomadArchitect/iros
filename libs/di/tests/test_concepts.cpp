#include <di/concepts/const.h>
#include <di/concepts/floating_point.h>
#include <di/concepts/language_array.h>
#include <di/concepts/member_function_pointer.h>
#include <di/concepts/member_object_pointer.h>
#include <di/concepts/reference.h>
#include <di/types/prelude.h>

namespace concepts {
static_assert(di::concepts::Reference<i32&&>);
static_assert(di::concepts::Const<i32 const>);

static_assert(di::concepts::OneOf<i32, i8, i16, i32, i64>);

static_assert(di::concepts::LanguageArray<int[32]>);
static_assert(di::concepts::LanguageArray<int[]>);
static_assert(!di::concepts::LanguageArray<int*>);

struct X {
    int y;
    int z();
};

static_assert(!di::concepts::MemberFunctionPointer<decltype(&X::y)>);
static_assert(di::concepts::MemberFunctionPointer<decltype(&X::z)>);
static_assert(di::concepts::MemberObjectPointer<decltype(&X::y)>);
static_assert(!di::concepts::MemberObjectPointer<decltype(&X::z)>);

static_assert(di::concepts::Integral<i32>);
static_assert(di::concepts::Integral<unsigned long long const volatile>);

static_assert(di::concepts::FloatingPoint<float>);
static_assert(di::concepts::FloatingPoint<long double const volatile>);
}
