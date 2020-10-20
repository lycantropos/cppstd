from typing import (List,
                    Tuple)

from hypothesis import strategies

from cppstd.hints import (Item,
                          Key)
from tests.utils import (BoundPortedMapsPair,
                         Strategy,
                         item_to_key,
                         to_bound_ported_maps_pair)

keys = values = strategies.integers()
empty_lists = strategies.builds(list)
items = strategies.tuples(keys, values)
items_lists = strategies.lists(items)
non_empty_items_lists = strategies.lists(items,
                                         min_size=1)
maps_pairs = strategies.builds(to_bound_ported_maps_pair,
                               items_lists)
empty_maps_pairs = strategies.builds(to_bound_ported_maps_pair,
                                     empty_lists)
non_empty_maps_pairs = strategies.builds(to_bound_ported_maps_pair,
                                         non_empty_items_lists)


def to_non_empty_maps_pairs_with_their_keys(
        items_list: List[Item]) -> Strategy[Tuple[BoundPortedMapsPair, Key]]:
    pair = to_bound_ported_maps_pair(items_list)
    return strategies.tuples(strategies.just(pair),
                             strategies.sampled_from(items_list)
                             .map(item_to_key))


non_empty_maps_pairs_with_their_keys = non_empty_items_lists.flatmap(
        to_non_empty_maps_pairs_with_their_keys)


def to_maps_pairs_with_non_their_keys(
        items_list: List[Item]) -> Strategy[Tuple[BoundPortedMapsPair, Key]]:
    pair = to_bound_ported_maps_pair(items_list)
    keys_list = list(map(item_to_key, items_list))
    return strategies.tuples(strategies.just(pair),
                             keys.filter(lambda candidate
                                         : candidate not in keys_list)
                             if items_list
                             else keys)


maps_pairs_with_non_their_keys = (items_lists
                                  .flatmap(to_maps_pairs_with_non_their_keys))
