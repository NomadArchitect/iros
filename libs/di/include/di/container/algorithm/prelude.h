#pragma once

#include <di/container/algorithm/adjacent_find.h>
#include <di/container/algorithm/all_of.h>
#include <di/container/algorithm/any_of.h>
#include <di/container/algorithm/compare.h>
#include <di/container/algorithm/contains.h>
#include <di/container/algorithm/contains_subrange.h>
#include <di/container/algorithm/copy.h>
#include <di/container/algorithm/copy_backward.h>
#include <di/container/algorithm/copy_if.h>
#include <di/container/algorithm/copy_n.h>
#include <di/container/algorithm/count.h>
#include <di/container/algorithm/count_if.h>
#include <di/container/algorithm/destroy.h>
#include <di/container/algorithm/destroy_n.h>
#include <di/container/algorithm/ends_with.h>
#include <di/container/algorithm/equal.h>
#include <di/container/algorithm/fill.h>
#include <di/container/algorithm/fill_n.h>
#include <di/container/algorithm/find.h>
#include <di/container/algorithm/find_end.h>
#include <di/container/algorithm/find_first_not_of.h>
#include <di/container/algorithm/find_first_of.h>
#include <di/container/algorithm/find_if.h>
#include <di/container/algorithm/find_if_not.h>
#include <di/container/algorithm/find_last.h>
#include <di/container/algorithm/find_last_if.h>
#include <di/container/algorithm/find_last_if_not.h>
#include <di/container/algorithm/find_last_not_of.h>
#include <di/container/algorithm/find_last_of.h>
#include <di/container/algorithm/fold_left.h>
#include <di/container/algorithm/fold_left_first.h>
#include <di/container/algorithm/fold_left_first_with_iter.h>
#include <di/container/algorithm/fold_left_with_iter.h>
#include <di/container/algorithm/fold_right.h>
#include <di/container/algorithm/fold_right_last.h>
#include <di/container/algorithm/for_each.h>
#include <di/container/algorithm/for_each_n.h>
#include <di/container/algorithm/generate.h>
#include <di/container/algorithm/generate_n.h>
#include <di/container/algorithm/iota.h>
#include <di/container/algorithm/is_heap.h>
#include <di/container/algorithm/is_heap_until.h>
#include <di/container/algorithm/is_partitioned.h>
#include <di/container/algorithm/is_permutation.h>
#include <di/container/algorithm/is_sorted.h>
#include <di/container/algorithm/is_sorted_until.h>
#include <di/container/algorithm/make_heap.h>
#include <di/container/algorithm/max.h>
#include <di/container/algorithm/max_element.h>
#include <di/container/algorithm/min.h>
#include <di/container/algorithm/min_element.h>
#include <di/container/algorithm/minmax.h>
#include <di/container/algorithm/minmax_element.h>
#include <di/container/algorithm/mismatch.h>
#include <di/container/algorithm/move_backward.h>
#include <di/container/algorithm/next_permutation.h>
#include <di/container/algorithm/none_of.h>
#include <di/container/algorithm/partition.h>
#include <di/container/algorithm/partition_copy.h>
#include <di/container/algorithm/partition_point.h>
#include <di/container/algorithm/pop_heap.h>
#include <di/container/algorithm/prev_permutation.h>
#include <di/container/algorithm/product.h>
#include <di/container/algorithm/push_heap.h>
#include <di/container/algorithm/remove.h>
#include <di/container/algorithm/remove_copy.h>
#include <di/container/algorithm/remove_copy_if.h>
#include <di/container/algorithm/remove_if.h>
#include <di/container/algorithm/replace.h>
#include <di/container/algorithm/replace_copy.h>
#include <di/container/algorithm/replace_copy_if.h>
#include <di/container/algorithm/replace_if.h>
#include <di/container/algorithm/reverse.h>
#include <di/container/algorithm/reverse_copy.h>
#include <di/container/algorithm/rotate.h>
#include <di/container/algorithm/rotate_copy.h>
#include <di/container/algorithm/search.h>
#include <di/container/algorithm/search_n.h>
#include <di/container/algorithm/shift_left.h>
#include <di/container/algorithm/shift_right.h>
#include <di/container/algorithm/shuffle.h>
#include <di/container/algorithm/sort.h>
#include <di/container/algorithm/sort_heap.h>
#include <di/container/algorithm/stable_partition.h>
#include <di/container/algorithm/starts_with.h>
#include <di/container/algorithm/sum.h>
#include <di/container/algorithm/swap_ranges.h>
#include <di/container/algorithm/transform.h>
#include <di/container/algorithm/uninitialized_copy.h>
#include <di/container/algorithm/uninitialized_copy_n.h>
#include <di/container/algorithm/uninitialized_default_construct.h>
#include <di/container/algorithm/uninitialized_default_construct_n.h>
#include <di/container/algorithm/uninitialized_fill.h>
#include <di/container/algorithm/uninitialized_fill_n.h>
#include <di/container/algorithm/uninitialized_move.h>
#include <di/container/algorithm/uninitialized_move_n.h>
#include <di/container/algorithm/uninitialized_relocate.h>
#include <di/container/algorithm/uninitialized_relocate_backwards.h>
#include <di/container/algorithm/uninitialized_value_construct.h>
#include <di/container/algorithm/uninitialized_value_construct_n.h>
#include <di/container/algorithm/unique.h>
#include <di/container/algorithm/unique_copy.h>

namespace di {
using container::adjacent_find;
using container::all_of;
using container::any_of;
using container::contains;
using container::contains_subrange;
using container::copy;
using container::copy_backward;
using container::copy_if;
using container::copy_n;
using container::count;
using container::count_if;
using container::destroy;
using container::destroy_n;
using container::ends_with;
using container::fill;
using container::fill_n;
using container::find;
using container::find_end;
using container::find_first_not_of;
using container::find_first_of;
using container::find_if;
using container::find_if_not;
using container::find_last;
using container::find_last_if;
using container::find_last_if_not;
using container::find_last_not_of;
using container::find_last_of;
using container::fold_left;
using container::fold_left_first;
using container::fold_left_first_with_iter;
using container::fold_left_with_iter;
using container::fold_right;
using container::fold_right_last;
using container::for_each;
using container::for_each_n;
using container::is_heap;
using container::is_heap_until;
using container::is_partitioned;
using container::is_permutation;
using container::is_sorted;
using container::is_sorted_until;
using container::make_heap;
using container::max;
using container::max_element;
using container::min;
using container::min_element;
using container::minmax;
using container::minmax_element;
using container::mismatch;
using container::next_permutation;
using container::none_of;
using container::partition;
using container::partition_copy;
using container::partition_point;
using container::pop_heap;
using container::prev_permutation;
using container::product;
using container::push_heap;
using container::remove_copy;
using container::remove_copy_if;
using container::replace;
using container::replace_copy;
using container::replace_copy_if;
using container::replace_if;
using container::reverse_copy;
using container::rotate;
using container::rotate_copy;
using container::search;
using container::search_n;
using container::shift_left;
using container::shift_right;
using container::shuffle;
using container::sort;
using container::sort_heap;
using container::stable_partition;
using container::starts_with;
using container::sum;
using container::swap_ranges;
using container::uninitialized_copy;
using container::uninitialized_copy_n;
using container::uninitialized_default_construct;
using container::uninitialized_default_construct_n;
using container::uninitialized_fill;
using container::uninitialized_fill_n;
using container::uninitialized_move;
using container::uninitialized_move_n;
using container::uninitialized_relocate;
using container::uninitialized_relocate_backwards;
using container::uninitialized_value_construct;
using container::uninitialized_value_construct_n;
using container::unique;
using container::unique_copy;
}
