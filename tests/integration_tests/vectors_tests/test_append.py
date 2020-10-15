from typing import Any

from hypothesis import given

from tests.utils import (BoundPortedVectorsPair,
                         are_bound_ported_vectors_equal)
from . import strategies


@given(strategies.non_empty_vectors_pairs, strategies.objects)
def test_basic(pair: BoundPortedVectorsPair, value: Any) -> None:
    bound, ported = pair

    bound_result, ported_result = bound.append(value), ported.append(value)

    assert bound_result is ported_result is None
    assert are_bound_ported_vectors_equal(bound, ported)
