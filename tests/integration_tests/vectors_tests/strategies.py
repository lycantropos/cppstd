import sys
from typing import Tuple

from hypothesis import strategies

from tests.utils import (BoundPortedVectorsPair,
                         Strategy,
                         to_bound_ported_vectors_pair)

MAX_INDEX = sys.maxsize
MIN_INDEX = -MAX_INDEX - 1
indices = strategies.integers(MIN_INDEX, MAX_INDEX)
sizes = strategies.integers(0, min(100, sys.maxsize))
invalid_sizes = strategies.integers(MIN_INDEX, -1)
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
    size = bound.size()
    return strategies.tuples(strategies.just(pair),
                             strategies.integers(-size, size - 1))


non_empty_vectors_pairs_with_indices = non_empty_vectors_pairs.flatmap(
        to_non_empty_vectors_pairs_with_indices)


def to_vectors_pairs_with_invalid_indices(
        pair: BoundPortedVectorsPair
) -> Strategy[Tuple[BoundPortedVectorsPair, int]]:
    bound, _ = pair
    size = bound.size()
    return strategies.tuples(strategies.just(pair),
                             strategies.integers(MIN_INDEX, -size - 1)
                             | strategies.integers(size + 1, MAX_INDEX))


vectors_pairs_with_invalid_indices = vectors_pairs.flatmap(
        to_vectors_pairs_with_invalid_indices)
