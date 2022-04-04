#pragma once

#include <assert.h>
#include <liim/tuple.h>
#include <liim/type_list.h>
#include <liim/utilities.h>
#include <stddef.h>

namespace LIIM {

using LIIM::Conditional;
using LIIM::FalseType;
using LIIM::IsConvertible;
using LIIM::TrueType;

namespace details {
    template<typename T, size_t... place>
    struct Array;
    template<typename T>
    struct Array<T> {
        constexpr const T& access() const { return m_value; }

        T m_value;
    };
    template<typename R, typename Visitor, typename... Variants, size_t head, size_t... tail>
    struct Array<R (*)(Visitor, Variants...), head, tail...> {
        static constexpr size_t index_in_array = sizeof...(Variants) - sizeof...(tail) - 1;

        using FunctionType = R (*)(Visitor, Variants...);

        template<typename... Args>
        constexpr const FunctionType& access(size_t first_index, Args... rest) const {
            return m_array[first_index].access(rest...);
        }

        Array<FunctionType, tail...> m_array[head];
    };

    template<typename ArrayType, typename VariantTuple, typename IndexSequence>
    struct ArrayBuilderImpl;
    template<typename R, typename Visitor, size_t... dimensions, typename... Variants, size_t... indicies>
    struct ArrayBuilderImpl<Array<R (*)(Visitor, Variants...), dimensions...>, Tuple<Variants...>, IndexSequence<indicies...>> {
        static constexpr size_t variant_index = sizeof...(indicies);
        using VariantType = typename RemoveReference<typename TypeList::TypeAtIndex<variant_index, Variants...>::type>::type;
        using ArrayType = Array<R (*)(Visitor, Variants...), dimensions...>;

        static constexpr ArrayType apply() {
            ArrayType table {};
            apply_all(table, LIIM::make_index_sequence<VariantType::num_variants()>());
            return table;
        }

        template<size_t... variant_indicies>
        static constexpr void apply_all(ArrayType& table, IndexSequence<variant_indicies...>) {
            (apply_one<variant_indicies>(table, table.m_array[variant_indicies]), ...);
        }

        template<size_t index, typename T>
        static constexpr void apply_one(ArrayType&, T& element) {
            element = ArrayBuilderImpl<typename RemoveReference<decltype(element)>::type, Tuple<Variants...>,
                                       IndexSequence<indicies..., index>>::apply();
        }
    };

    template<typename R, typename Visitor, typename... Variants, size_t... indices>
    struct ArrayBuilderImpl<Array<R (*)(Visitor, Variants...)>, Tuple<Variants...>, IndexSequence<indices...>> {
        using ArrayType = Array<R (*)(Visitor, Variants...)>;

        template<size_t index, typename Variant>
        static constexpr decltype(auto) element_in_variant_by_index(Variant&& var) {
            return forward<Variant>(var).get(in_place_index<index>);
        }

        static constexpr decltype(auto) visit_invoke_impl(Visitor&& visitor, Variants... vars) {
            if constexpr (IsVoid<R>::value) {
                return (void) LIIM::invoke(forward<Visitor>(visitor), element_in_variant_by_index<indices>(forward<Variants>(vars))...);
            } else
                return LIIM::invoke(forward<Visitor>(visitor), element_in_variant_by_index<indices>(forward<Variants>(vars))...);
        }

        static constexpr decltype(auto) do_visit_invoke(Visitor&& visitor, Variants... vars) {
            return visit_invoke_impl(forward<Visitor>(visitor), forward<Variants>(vars)...);
        }

        static constexpr decltype(auto) visit_invoke(Visitor&& visitor, Variants... vars) {
            return do_visit_invoke(forward<Visitor>(visitor), forward<Variants>(vars)...);
        }

        static constexpr auto apply() { return ArrayType { &visit_invoke }; }
    };

    template<typename R, typename Visitor, typename... Variants>
    struct ArrayBuilder {
        using ArrayType = Array<R (*)(Visitor, Variants...), (RemoveReference<Variants>::type::num_variants())...>;

        static constexpr ArrayType table = ArrayBuilderImpl<ArrayType, Tuple<Variants...>, IndexSequence<>>::apply();
    };
}

template<typename Visitor, typename... Variants>
inline constexpr decltype(auto) visit(Visitor&& vis, Variants&&...);

template<typename... Types>
class Variant {
public:
    Variant() {
        using FirstType = typename TypeList::First<Types...>::type;
        new (&m_value_storage[0]) FirstType();
    }

    Variant(const Variant& other) : m_value_index(other.m_value_index) {
        const_cast<Variant&>(other).visit([&](auto&& value) {
            using RealType = decay_t<decltype(value)>;
            new (&m_value_storage[0]) RealType(value);
        });
    }

    Variant(Variant&& other) : m_value_index(other.m_value_index) {
        other.visit([&](auto&& value) {
            using RealType = decay_t<decltype(value)>;
            new (&m_value_storage[0]) RealType(LIIM::move(value));
        });
    }

    template<typename T, typename... Args>
    Variant(in_place_type_t<T>, Args... args, typename EnableIf<TypeList::Index<T, Types...>::value != -1>::type* = 0) {
        constexpr size_t index = TypeList::Index<T, Types...>::value;
        static_assert(index != -1);
        new (&m_value_storage[0]) T(forward<Args>(args)...);
        m_value_index = index;
    }

    template<size_t index, typename... Args>
    Variant(in_place_index_t<index>, Args... args) {
        using RealType = typename TypeList::TypeAtIndex<index, Types...>::type;
        new (&m_value_storage[0]) RealType(forward<Args>(args)...);
        m_value_index = index;
    }

    template<typename T>
    Variant(const T& other, typename EnableIf<TypeList::IsValid<T, Types...>::value>::type* = 0) {
        using RealType = typename TypeList::IsValid<T, Types...>::type;
        constexpr size_t index = TypeList::Index<RealType, Types...>::value;
        static_assert(index != -1);
        new (&m_value_storage[0]) RealType(other);
        m_value_index = index;
    }

    template<typename T>
    Variant(T&& other, typename EnableIf<TypeList::IsValid<T, Types...>::value>::type* = 0) {
        using RealType = typename TypeList::IsValid<T, Types...>::type;
        constexpr size_t index = TypeList::Index<RealType, Types...>::value;
        static_assert(index != -1);
        new (&m_value_storage[0]) RealType(move(other));
        m_value_index = index;
    }

    ~Variant() { destroy(); }

    Variant& operator=(const Variant& other) {
        if (this != &other) {
            Variant temp(other);
            swap(temp);
        }
        return *this;
    }
    Variant& operator=(Variant&& other) {
        if (this != &other) {
            Variant temp(LIIM::move(other));
            swap(temp);
        }
        return *this;
    }
    template<typename T, typename = typename EnableIf<TypeList::IsValid<T, Types...>::value>::type>
    Variant& operator=(const T& value) {
        Variant temp(value);
        swap(temp);
        return *this;
    }
    template<typename T, typename = typename EnableIf<TypeList::IsValid<T, Types...>::value>::type>
    Variant& operator=(T&& value) {
        Variant temp(LIIM::move(value));
        swap(temp);
        return *this;
    }

    template<typename T>
    constexpr T* get_if() {
        constexpr size_t index = TypeList::Index<T, Types...>::value;
        static_assert(index != -1);
        if (m_value_index != index) {
            return nullptr;
        }
        return &this->get<index>();
    }
    template<typename T>
    constexpr const T* get_if() const {
        constexpr size_t index = TypeList::Index<T, Types...>::value;
        static_assert(index != -1);
        if (m_value_index != index) {
            return nullptr;
        }
        return &this->get<index>();
    }

    template<typename T>
    constexpr T get_or(T value) {
        constexpr size_t index = TypeList::Index<T, Types...>::value;
        static_assert(index != -1);
        if (m_value_index != index) {
            return value;
        }
        return this->get<index>();
    }

    template<size_t index>
    constexpr typename TypeList::TypeAtIndex<index, Types...>::type& get(in_place_index_t<index>) {
        return this->get<index>();
    }
    template<size_t index>
    constexpr const typename TypeList::TypeAtIndex<index, Types...>::type& get(in_place_index_t<index>) const {
        return this->get<index>();
    }

    template<size_t index>
    constexpr typename TypeList::TypeAtIndex<index, Types...>::type& get() {
        assert(m_value_index == index);
        using RealType = typename TypeList::TypeAtIndex<index, Types...>::type;
        return *reinterpret_cast<RealType*>(&m_value_storage[0]);
    }
    template<size_t index>
    constexpr const typename TypeList::TypeAtIndex<index, Types...>::type& get() const {
        return const_cast<Variant&>(*this).get<index>();
    }

    template<typename T>
    constexpr T& as() {
        constexpr size_t index = TypeList::Index<T, Types...>::value;
        static_assert(index != -1);
        return this->get<index>();
    }
    template<typename T>
    constexpr const T& as() const {
        return const_cast<Variant&>(*this).as<T>();
    }

    template<size_t index>
    constexpr bool is() const {
        return m_value_index == index;
    }

    template<typename T>
    constexpr bool is() const {
        constexpr size_t index = TypeList::Index<T, Types...>::value;
        static_assert(index != -1);
        return this->is<index>();
    }

    template<typename Visitor>
    constexpr decltype(auto) visit(Visitor&& vis) {
        return LIIM::visit(forward<Visitor>(vis), forward<Variant>(*this));
    }

    template<typename T, typename... Args, typename = typename EnableIf<TypeList::Index<T, Types...>::value != -1>::type>
    void emplace(Args&&... args) {
        this->destroy();
        new (&m_value_storage[0]) T(forward<Args>(args)...);
        m_value_index = TypeList::Index<T, Types...>::value;
    }

    constexpr size_t index() const { return m_value_index; }

    void swap(Variant& other) {
        this->visit([&](auto&& a) {
            auto temp(LIIM::move(a));
            using TempType = decay_t<decltype(a)>;
            other.visit([&](auto&& b) {
                using OtherType = decay_t<decltype(b)>;

                this->destroy();
                new (&m_value_storage[0]) OtherType(LIIM::move(b));

                other.destroy();
                new (&other.m_value_storage) TempType(LIIM::move(temp));
            });
        });
        LIIM::swap(this->m_value_index, other.m_value_index);
    }

    static constexpr size_t num_variants() { return TypeList::Count<Types...>::value; }

    bool operator!=(const Variant& other) const { return !(*this == other); }
    bool operator==(const Variant& other) const {
        return this->m_value_index == other.m_value_index && LIIM::visit(
                                                                 [&](auto&& a, auto&& b) -> bool {
                                                                     using A = LIIM::decay_t<decltype(a)>;
                                                                     using B = LIIM::decay_t<decltype(b)>;
                                                                     if constexpr (!IsSame<A, B>::value) {
                                                                         return false;
                                                                     } else {
                                                                         return a == b;
                                                                     }
                                                                 },
                                                                 forward<const Variant>(*this), forward<const Variant>(other));
    }
    bool operator<=(const Variant& other) const {
        return LIIM::visit(
            [&](auto&& a, auto&& b) -> bool {
                return a <= b;
            },
            forward<const Variant>(*this), forward<const Variant>(other));
    }
    bool operator>=(const Variant& other) const {
        return LIIM::visit(
            [&](auto&& a, auto&& b) -> bool {
                return a >= b;
            },
            forward<const Variant>(*this), forward<const Variant>(other));
    }
    bool operator<(const Variant& other) const {
        return LIIM::visit(
            [&](auto&& a, auto&& b) -> bool {
                return a < b;
            },
            forward<const Variant>(*this), forward<const Variant>(other));
    }
    bool operator>(const Variant& other) const {
        return LIIM::visit(
            [&](auto&& a, auto&& b) -> bool {
                return a > b;
            },
            forward<const Variant>(*this), forward<const Variant>(other));
    }

private:
    template<typename Visitor, typename... Variants>
    friend inline constexpr decltype(auto) visit(Visitor&&, Variants&&...);

    void destroy() {
        this->visit([this](auto&& val) {
            using T = decay_t<decltype(val)>;
            forward<T>(val).~T();
        });
    }

    unsigned char m_value_storage[TypeList::Size<Types...>::value];
    size_t m_value_index { 0 };
    static_assert(sizeof(m_value_storage) >= TypeList::Size<Types...>::value);
};

template<typename... Types>
void swap(Variant<Types...>& a, Variant<Types...>& b) {
    a.swap(b);
}

template<typename Visitor, typename... Variants>
inline constexpr decltype(auto) visit(Visitor&& vis, Variants&&... vs) {
    using R = typename InvokeResult<Visitor, decltype(LIIM::declval<Variants>().get(in_place_index<0>))...>::type;

    constexpr auto& table = details::ArrayBuilder<R, Visitor&&, Variants&&...>::table;

    auto func_ptr = table.access(vs.index()...);
    assert(func_ptr != nullptr);
    return (*func_ptr)(forward<Visitor>(vis), forward<Variants>(vs)...);
}

struct Monostate {
    bool operator==(const Monostate&) const { return true; }
    bool operator!=(const Monostate&) const { return false; }
};

}

using LIIM::Monostate;
using LIIM::Variant;
using LIIM::visit;
