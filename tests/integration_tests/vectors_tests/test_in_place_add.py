from typing import (Any,
                    List)

from hypothesis import given

from tests.utils import (BoundPortedVectorsPair,
                         are_bound_ported_vectors_equal)
from . import strategies


@given(strategies.vectors_pairs, strategies.objects_lists)
def test_basic(pair: BoundPortedVectorsPair, values: List[Any]) -> None:
    bound, ported = pair

    bound += values
    ported += values

    assert are_bound_ported_vectors_equal(bound, ported)
