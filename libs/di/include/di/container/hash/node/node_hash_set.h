#pragma once

#include <di/container/allocator/prelude.h>
#include <di/container/associative/set_interface.h>
#include <di/container/concepts/prelude.h>
#include <di/container/hash/default_hasher.h>
#include <di/container/hash/node/hash_node.h>
#include <di/container/hash/node/owning_node_hash_table.h>
#include <di/container/intrusive/forward_list_forward_declaration.h>
#include <di/container/vector/mutable_vector.h>
#include <di/container/vector/vector.h>
#include <di/container/view/transform.h>
#include <di/function/compare.h>
#include <di/function/equal.h>
#include <di/platform/prelude.h>
#include <di/util/deduce_create.h>
#include <di/vocab/optional/prelude.h>

namespace di::container {
namespace detail {
    template<typename Value>
    struct NodeHashSetTag : OwningHashNodeTag<NodeHashSetTag<Value>, Value> {};
}

template<typename Value, typename Eq = function::Equal, concepts::Hasher Hasher = DefaultHasher,
         typename Buckets = container::Vector<
             IntrusiveForwardList<HashNode<detail::NodeHashSetTag<Value>>, detail::NodeHashSetTag<Value>>>,
         concepts::AllocatorOf<OwningHashNode<Value, detail::NodeHashSetTag<Value>>> Alloc =
             DefaultAllocator<OwningHashNode<Value, detail::NodeHashSetTag<Value>>>>
class NodeHashSet
    : public OwningNodeHashTable<
          Value, Eq, Hasher, Buckets, detail::NodeHashSetTag<Value>, Alloc,
          SetInterface<NodeHashSet<Value, Eq, Hasher, Buckets, Alloc>, Value,
                       HashNodeIterator<Value, detail::NodeHashSetTag<Value>>,
                       meta::ConstIterator<HashNodeIterator<Value, detail::NodeHashSetTag<Value>>>,
                       detail::NodeHashTableValidForLookup<Value, Eq>::template Type, false>,
          false> {
private:
    using Base =
        OwningNodeHashTable<Value, Eq, Hasher, Buckets, detail::NodeHashSetTag<Value>, Alloc,
                            SetInterface<NodeHashSet<Value, Eq, Hasher, Buckets, Alloc>, Value,
                                         HashNodeIterator<Value, detail::NodeHashSetTag<Value>>,
                                         meta::ConstIterator<HashNodeIterator<Value, detail::NodeHashSetTag<Value>>>,
                                         detail::NodeHashTableValidForLookup<Value, Eq>::template Type, false>,
                            false>;

public:
    using Base::Base;
};

template<concepts::InputContainer Con, typename T = meta::ContainerValue<Con>>
NodeHashSet<T> tag_invoke(types::Tag<util::deduce_create>, InPlaceTemplate<NodeHashSet>, Con&&);

template<concepts::InputContainer Con, typename T = meta::ContainerValue<Con>, typename Eq>
NodeHashSet<T, Eq> tag_invoke(types::Tag<util::deduce_create>, InPlaceTemplate<NodeHashSet>, Con&&, Eq);

template<concepts::InputContainer Con, typename T = meta::ContainerValue<Con>, typename Eq, typename Hasher>
NodeHashSet<T, Eq, Hasher> tag_invoke(types::Tag<util::deduce_create>, InPlaceTemplate<NodeHashSet>, Con&&, Eq, Hasher);
}
