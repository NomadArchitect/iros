#pragma once

#include <di/concepts/arithmetic.h>
#include <di/concepts/enum.h>
#include <di/concepts/pointer.h>
#include <di/meta/conditional.h>
#include <di/sync/memory_order.h>
#include <di/types/prelude.h>
#include <di/util/address_of.h>
#include <di/util/to_underlying.h>

namespace di::sync {
template<typename T>
requires((concepts::Enum<T> || concepts::Arithmetic<T> || concepts::Pointer<T>) && sizeof(T) <= sizeof(void*))
class AtomicRef {
private:
    using DeltaType = meta::Conditional<concepts::Pointer<T>, ptrdiff_t, T>;

    // NOTE: the builtin atomic operations treat pointer addition bytewise, so we
    //       must multiply by the sizeof(*T) if T is a pointer.
    constexpr DeltaType adjust_delta(DeltaType value) {
        if constexpr (concepts::Pointer<T>) {
            return value * sizeof(**m_pointer);
        } else {
            return value;
        }
    }

public:
    AtomicRef(AtomicRef const&) = default;

    constexpr explicit AtomicRef(T& value) : m_pointer(util::address_of(value)) {}

    AtomicRef& operator=(AtomicRef const&) = delete;

    void store(T value, MemoryOrder order = MemoryOrder::SequentialConsistency) {
        __atomic_store_n(m_pointer, value, util::to_underlying(order));
    }

    T load(MemoryOrder order = MemoryOrder::SequentialConsistency) const {
        return __atomic_load_n(m_pointer, util::to_underlying(order));
    }

    T exchange(T value, MemoryOrder order = MemoryOrder::SequentialConsistency) {
        return __atomic_exchange_n(m_pointer, value, util::to_underlying(order));
    }

    bool compare_exchange_weak(T& expected, T desired, MemoryOrder success, MemoryOrder failure) {
        return __atomic_compare_exchange_n(m_pointer, util::address_of(expected), desired, true,
                                           util::to_underlying(success), util::to_underlying(failure));
    }

    bool compare_exchange_weak(T& expected, T desired, MemoryOrder order = MemoryOrder::SequentialConsistency) {
        if (order == MemoryOrder::AcquireRelease || order == MemoryOrder::Release) {
            return compare_exchange_weak(exchange, desired, MemoryOrder::Release, MemoryOrder::Acquire);
        } else {
            return compare_exchange_weak(expected, desired, order, order);
        }
    }

    bool compare_exchange_strong(T& expected, T desired, MemoryOrder success, MemoryOrder failure) {
        return __atomic_compare_exchange_n(m_pointer, util::address_of(expected), desired, false,
                                           util::to_underlying(success), util::to_underlying(failure));
    }

    bool compare_exchange_strong(T& expected, T desired, MemoryOrder order = MemoryOrder::SequentialConsistency) {
        if (order == MemoryOrder::AcquireRelease || order == MemoryOrder::Release) {
            return compare_exchange_strong(exchange, desired, MemoryOrder::Release, MemoryOrder::Acquire);
        } else {
            return compare_exchange_strong(expected, desired, order, order);
        }
    }

    constexpr T fetch_add(DeltaType delta, MemoryOrder order = MemoryOrder::SequentialConsistency)
    requires(concepts::Integral<T> || concepts::Pointer<T>)
    {
        return __atomic_add_fetch(m_pointer, adjust_delta(delta), util::to_underlying(order));
    }

    constexpr T fetch_sub(DeltaType delta, MemoryOrder order = MemoryOrder::SequentialConsistency)
    requires(concepts::Integral<T> || concepts::Pointer<T>)
    {
        return __atomic_sub_fetch(m_pointer, adjust_delta(delta), util::to_underlying(order));
    }

    constexpr T fetch_and(T value, MemoryOrder order = MemoryOrder::SequentialConsistency)
    requires(concepts::Integral<T>)
    {
        return __atomic_and_fetch(m_pointer, value, util::to_underlying(order));
    }

    constexpr T fetch_or(T value, MemoryOrder order = MemoryOrder::SequentialConsistency)
    requires(concepts::Integral<T>)
    {
        return __atomic_or_fetch(m_pointer, value, util::to_underlying(order));
    }

    constexpr T fetch_xor(T value, MemoryOrder order = MemoryOrder::SequentialConsistency)
    requires(concepts::Integral<T>)
    {
        return __atomic_xor_fetch(m_pointer, value, util::to_underlying(order));
    }

private:
    T* m_pointer { nullptr };
};
}