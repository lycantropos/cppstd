from typing import Any

from hypothesis import given

from tests.utils import (BoundPortedVectorsPair,
                         are_bound_ported_vectors_equal)
from . import strategies


@given(strategies.non_empty_vectors_pairs, strategies.indices,
       strategies.objects)
def test_basic(pair: BoundPortedVectorsPair, index: int, value: Any) -> None:
    bound, ported = pair

    bound_result, ported_result = (bound.insert(index, value),
                                   ported.insert(index, value))

    assert bound_result is ported_result is None
    assert are_bound_ported_vectors_equal(bound, ported)
