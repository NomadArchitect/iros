#include <di/container/interface/begin.h>
#include <di/container/interface/end.h>
#include <di/container/view/owning_view.h>
#include <di/container/view/ref_view.h>
#include <di/container/view/view.h>
#include <test/test.h>

constexpr void basic() {
    int arr[] = { 1, 2, 3, 4, 5 };
    auto x = di::container::View { di::container::begin(arr), di::container::end(arr) };

    auto [s, e] = x;
    EXPECT_EQ(s, arr + 0);
    EXPECT_EQ(e, arr + 5);

    {
        auto sum = 0;
        for (auto z : x) {
            sum += z;
        }
        EXPECT_EQ(sum, 15);
    }
    {
        x.advance(2);
        auto sum = 0;
        for (auto z : x) {
            sum += z;
        }
        EXPECT_EQ(sum, 12);
    }
}

TEST_CONSTEXPR(container_view, basic, basic)
