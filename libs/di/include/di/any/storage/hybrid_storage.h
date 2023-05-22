#pragma once

#include <di/any/concepts/vtable_for.h>
#include <di/any/storage/storage_category.h>
#include <di/concepts/convertible_to.h>
#include <di/concepts/move_constructible.h>
#include <di/concepts/object.h>
#include <di/concepts/reference.h>
#include <di/container/allocator/allocate_one.h>
#include <di/container/allocator/allocator.h>
#include <di/container/allocator/deallocate_one.h>
#include <di/meta/list/prelude.h>
#include <di/platform/prelude.h>
#include <di/types/prelude.h>
#include <di/util/addressof.h>
#include <di/util/construct_at.h>
#include <di/util/destroy_at.h>
#include <di/util/exchange.h>
#include <di/util/move.h>
#include <di/vocab/expected/as_fallible.h>
#include <di/vocab/expected/try_infallible.h>

namespace di::any {
namespace detail {
    template<typename HybridStorage, concepts::Allocator Alloc>
    struct HybridStorageManage {
        using Type = Method<HybridStorageManage, void(This&, HybridStorage*, HybridStorage*, Alloc&)>;

        template<typename T>
        void operator()(T&, HybridStorage*, HybridStorage*, Alloc&) const;
    };

    template<typename HybridStorage, concepts::Allocator Alloc>
    constexpr inline auto hybrid_storage_manage = HybridStorageManage<HybridStorage, Alloc> {};
}

template<size_t inline_size = 2 * sizeof(void*), size_t inline_align = alignof(void*),
         concepts::Allocator Alloc = platform::DefaultAllocator>
struct HybridStorage {
public:
    using Manage = meta::Type<detail::HybridStorageManage<HybridStorage, Alloc>>;
    using Interface = meta::List<Manage>;

    template<typename, concepts::Allocator>
    friend struct detail::HybridStorageManage;

    constexpr static StorageCategory storage_category() { return StorageCategory::MoveOnly; }

    template<typename T>
    constexpr static bool creation_is_inline(InPlaceType<T>) {
        return sizeof(T) <= inline_size && alignof(T) <= inline_align && concepts::MoveConstructible<T>;
    }

    template<typename T>
    constexpr static bool creation_is_fallible(InPlaceType<T>) {
        return !creation_is_inline(in_place_type<T>) && concepts::FallibleAllocator<Alloc>;
    }

    constexpr HybridStorage() {}

    HybridStorage(HybridStorage const&) = delete;
    HybridStorage& operator=(HybridStorage const&) = delete;

    template<typename T, typename... Args>
    requires(concepts::ConstructibleFrom<T, Args...>)
    constexpr static auto init(HybridStorage* self, InPlaceType<T>, Args&&... args) {
        if constexpr (!creation_is_inline(in_place_type<T>)) {
            return vocab::as_fallible(di::allocate_one<T>(self->m_allocator)) % [&](T* pointer) {
                util::construct_at(pointer, util::forward<Args>(args)...);
                self->m_pointer = pointer;
            } | vocab::try_infallible;
        } else {
            util::construct_at(self->down_cast<T>(), util::forward<Args>(args)...);
        }
    }

    ~HybridStorage() = default;

    constexpr static void move_construct(concepts::VTableFor<Interface> auto& vtable, HybridStorage* dest,
                                         HybridStorage* source) {
        if (!vtable.empty()) {
            auto const fp = vtable[Manage {}];
            fp(dest, dest, source, dest->m_allocator);

            vtable.reset();
        }
    }

    template<concepts::VTableFor<Interface> VTable>
    constexpr static void move_assign(VTable& dest_vtable, HybridStorage* dest, VTable& source_vtable,
                                      HybridStorage* source) {
        destroy(dest_vtable, dest);
        dest_vtable = source_vtable;
        move_construct(source_vtable, dest, source);
    }

    constexpr static void destroy(concepts::VTableFor<Interface> auto& vtable, HybridStorage* self) {
        if (!vtable.empty()) {
            auto const fp = vtable[Manage {}];
            fp(self, self, nullptr, self->m_allocator);

            vtable.reset();
        }
    }

    template<typename T>
    T* down_cast() {
        if constexpr (!creation_is_inline(in_place_type<T>)) {
            return static_cast<T*>(m_pointer);
        } else {
            return static_cast<T*>(address());
        }
    }

    template<typename T>
    T const* down_cast() const {
        if constexpr (!creation_is_inline(in_place_type<T>)) {
            return static_cast<T const*>(m_pointer);
        } else {
            return static_cast<T const*>(address());
        }
    }

private:
    void* address() { return static_cast<void*>(util::addressof(m_storage[0])); }
    void const* address() const { return static_cast<void const*>(util::addressof(m_storage[0])); }

    union {
        void* m_pointer;
        alignas(inline_align) byte m_storage[inline_size];
    };
    [[no_unique_address]] Alloc m_allocator {};
};

namespace detail {
    template<typename HybridStorage, concepts::Allocator Alloc>
    template<typename T>
    void HybridStorageManage<HybridStorage, Alloc>::operator()(T& a, HybridStorage* as, HybridStorage* b,
                                                               Alloc& allocator) const {
        if constexpr (!HybridStorage::creation_is_inline(in_place_type<T>)) {
            if (b) {
                // Move from b into a.
                as->m_pointer = util::exchange(b->m_pointer, nullptr);
            } else {
                // Just destroy a.
                auto* pointer = util::exchange(as->m_pointer, nullptr);
                auto* a_value = static_cast<T*>(pointer);
                util::destroy_at(a_value);
                di::deallocate_one<T>(allocator, a_value);
            }
        } else {
            if (b) {
                // Move from b into a.
                auto* b_value = b->template down_cast<T>();
                util::construct_at(util::addressof(a), util::move(*b_value));
                util::destroy_at(b_value);
            } else {
                // Just destroy a.
                util::destroy_at(util::addressof(a));
            }
        }
    }
}
}
