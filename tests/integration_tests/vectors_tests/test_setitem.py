from typing import (Any,
                    List,
                    Tuple)

from hypothesis import given

from tests.utils import (BoundPortedVectorsPair,
                         are_bound_ported_vectors_equal)
from . import strategies


@given(strategies.non_empty_vectors_pairs_with_indices, strategies.objects)
def test_index(pair_with_index: Tuple[BoundPortedVectorsPair, int],
               value: Any) -> None:
    (bound, ported), index = pair_with_index

    bound[index] = ported[index] = value

    assert are_bound_ported_vectors_equal(bound, ported)


@given(strategies.vectors_pairs_with_slices_and_objects_lists)
def test_slice(pair_with_slice_and_values
               : Tuple[BoundPortedVectorsPair, int, List[Any]]) -> None:
    (bound, ported), slice_, values = pair_with_slice_and_values

    bound[slice_] = ported[slice_] = values

    assert are_bound_ported_vectors_equal(bound, ported)
