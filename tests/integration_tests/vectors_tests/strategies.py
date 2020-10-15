import sys
from typing import (Any,
                    Callable,
                    List,
                    Tuple)

from hypothesis import strategies

from tests.utils import (BoundPortedVectorsPair,
                         Domain,
                         Strategy,
                         to_bound_ported_vectors_pair)

objects = strategies.integers()
empty_lists = strategies.builds(list)
objects_lists = strategies.lists(objects)
non_empty_objects_lists = strategies.lists(objects,
                                           min_size=1)
vectors_pairs = strategies.builds(to_bound_ported_vectors_pair,
                                  objects_lists)
empty_vectors_pairs = strategies.builds(to_bound_ported_vectors_pair,
                                        empty_lists)
non_empty_vectors_pairs = strategies.builds(to_bound_ported_vectors_pair,
                                            non_empty_objects_lists)


def to_non_empty_vectors_pairs_with_indices(
        pair: BoundPortedVectorsPair
) -> Strategy[Tuple[BoundPortedVectorsPair, int]]:
    bound, _ = pair
    size = len(bound)
    return strategies.tuples(strategies.just(pair),
                             strategies.integers(-size, size - 1))


non_empty_vectors_pairs_with_indices = (
    non_empty_vectors_pairs.flatmap(to_non_empty_vectors_pairs_with_indices))


def to_vectors_pairs_with_invalid_indices(
        pair: BoundPortedVectorsPair
) -> Strategy[Tuple[BoundPortedVectorsPair, int]]:
    bound, _ = pair
    size = len(bound)
    return strategies.tuples(strategies.just(pair),
                             strategies.integers(-sys.maxsize, -size - 1)
                             | strategies.integers(size + 1, sys.maxsize))


vectors_pairs_with_invalid_indices = (
    vectors_pairs.flatmap(to_vectors_pairs_with_invalid_indices))


def to_vectors_pairs_with_slices(
        pair: BoundPortedVectorsPair
) -> Strategy[Tuple[BoundPortedVectorsPair, slice]]:
    bound, _ = pair
    size = len(bound)
    return strategies.tuples(strategies.just(pair), strategies.slices(size))


vectors_pairs_with_slices = (vectors_pairs
                             .flatmap(to_vectors_pairs_with_slices))


@strategies.composite
def to_vectors_pairs_with_slices_and_iterables_pairs(
        draw: Callable[[Strategy[Domain]], Domain],
        pair: BoundPortedVectorsPair
) -> Strategy[Tuple[BoundPortedVectorsPair, slice, List[Any]]]:
    bound, _ = pair
    size = len(bound)
    slice_ = draw(strategies.slices(size))
    slice_size = len(bound[slice_])
    return pair, slice_, draw((objects_lists
                               if slice_.step == 1
                               else strategies.lists(objects,
                                                     min_size=slice_size,
                                                     max_size=slice_size))
                              if slice_size
                              else empty_lists)


vectors_pairs_with_slices_and_objects_lists = (
    vectors_pairs.flatmap(to_vectors_pairs_with_slices_and_iterables_pairs))
