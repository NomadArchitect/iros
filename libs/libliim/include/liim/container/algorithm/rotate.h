#pragma once

#include <liim/container/concepts.h>
#include <liim/container/producer/iterator_container.h>

namespace LIIM::Container::Algorithm {
template<MutableRandomAccessContainer C>
constexpr auto rotate(C&& container, IteratorForContainer<C> middle) {
    auto start = forward<C>(container).begin();
    auto end = forward<C>(container).end();

    auto to_rotate = end - middle;
    reverse(forward<C>(container));
    reverse(iterator_container(start, start + to_rotate));
    reverse(iterator_container(start + to_rotate, end));
    return start + to_rotate;
}
}

using LIIM::Container::Algorithm::rotate;