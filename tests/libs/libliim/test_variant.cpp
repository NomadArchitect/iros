#include <liim/variant.h>
#include <test/test.h>

constexpr auto xxx = [] {
    Variant<int, float, double> x;
    x = 4;
    return x.as<int>();
}();
static_assert(xxx == 4);

TEST(variant, basic) {
    Variant<int, float, double> v;

    static_assert(LIIM::IsSame<int&, decltype(v.get<0>())>::value);
    static_assert(LIIM::IsSame<float&, decltype(v.get<1>())>::value);
    static_assert(LIIM::IsSame<double&, decltype(v.get<2>())>::value);

    const Variant<int, float, long> w(4L);

    static_assert(LIIM::IsSame<const int&, decltype(w.get<0>())>::value);
    static_assert(LIIM::IsSame<const float&, decltype(w.get<1>())>::value);
    static_assert(LIIM::IsSame<const long&, decltype(w.get<2>())>::value);

    EXPECT(v.get_if<int>());
    EXPECT(!v.get_if<float>());

    EXPECT_EQ(v.as<int>(), 0);
    EXPECT_EQ(w.as<long>(), 4);
}

TEST(variant, assign) {
    Variant<int, float, double> v;
    v = 3;

    EXPECT_EQ(v.as<int>(), 3);
}

TEST(variant, references) {
    int s = 5;
    Variant<const int&, float&, double&> v(s);

    static_assert(LIIM::IsSame<const int&, decltype(v.get<0>())>::value);
    static_assert(LIIM::IsSame<float&, decltype(v.get<1>())>::value);
    static_assert(LIIM::IsSame<double&, decltype(v.get<2>())>::value);

    static_assert(LIIM::IsSame<const int&, decltype(v.get(in_place_index<0>))>::value);

    EXPECT(v.is<const int&>());
    EXPECT_EQ(v.as<const int&>(), 5);

    s = 42;
    EXPECT_EQ(v.get_if<const int&>().value(), 42);

    const int& x = 88;
    v = x;
    EXPECT_EQ(s, 42);
    EXPECT_EQ(v.as<const int&>(), 88);
}
