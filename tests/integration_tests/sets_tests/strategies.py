from typing import (Any,
                    List,
                    Tuple)

from hypothesis import strategies

from tests.utils import (BoundPortedSetsPair,
                         Strategy,
                         to_bound_ported_sets_pair)

objects = strategies.integers()
empty_lists = strategies.builds(list)
objects_lists = strategies.lists(objects)
non_empty_objects_lists = strategies.lists(objects,
                                           min_size=1)
sets_pairs = strategies.builds(to_bound_ported_sets_pair,
                               objects_lists)
empty_sets_pairs = strategies.builds(to_bound_ported_sets_pair,
                                     empty_lists)
non_empty_sets_pairs = strategies.builds(to_bound_ported_sets_pair,
                                         non_empty_objects_lists)


def to_non_empty_sets_pairs_with_their_elements(
        values: List[Any]) -> Strategy[Tuple[BoundPortedSetsPair, Any]]:
    pair = to_bound_ported_sets_pair(values)
    return strategies.tuples(strategies.just(pair),
                             strategies.sampled_from(values))


non_empty_sets_pairs_with_their_elements = non_empty_objects_lists.flatmap(
        to_non_empty_sets_pairs_with_their_elements)


def to_sets_pairs_with_non_their_elements(
        values: List[Any]) -> Strategy[Tuple[BoundPortedSetsPair, Any]]:
    pair = to_bound_ported_sets_pair(values)
    return strategies.tuples(strategies.just(pair),
                             objects.filter(lambda candidate
                                            : candidate not in values)
                             if values
                             else objects)


sets_pairs_with_non_their_elements = (
    objects_lists.flatmap(to_sets_pairs_with_non_their_elements))
