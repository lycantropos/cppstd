from typing import Tuple

from hypothesis import given

from tests.utils import (BoundPortedVectorsPair,
                         are_bound_ported_vectors_equal)
from . import strategies


@given(strategies.non_empty_vectors_pairs_with_indices)
def test_index(pair_with_index: Tuple[BoundPortedVectorsPair, int]) -> None:
    (bound, ported), index = pair_with_index

    bound_result, ported_result = bound[index], ported[index]

    assert bound_result == ported_result


@given(strategies.vectors_pairs_with_slices)
def test_slice(pair_with_slice: Tuple[BoundPortedVectorsPair, int]) -> None:
    (bound, ported), slice_ = pair_with_slice

    bound_result, ported_result = bound[slice_], ported[slice_]

    assert are_bound_ported_vectors_equal(bound_result, ported_result)
