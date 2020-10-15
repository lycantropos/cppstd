from hypothesis import given

from tests.utils import (BoundPortedVectorsPair,
                         are_bound_ported_vectors_equal)
from . import strategies


@given(strategies.vectors_pairs)
def test_basic(pair: BoundPortedVectorsPair) -> None:
    bound, ported = pair

    bound_result, ported_result = bound.clear(), ported.clear()

    assert bound_result is ported_result is None
    assert are_bound_ported_vectors_equal(bound, ported)
