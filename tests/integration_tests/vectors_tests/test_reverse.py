from hypothesis import given

from tests.utils import (BoundPortedVectorsPair,
                         are_bound_ported_vectors_equal)
from . import strategies


@given(strategies.non_empty_vectors_pairs)
def test_basic(pair: BoundPortedVectorsPair) -> None:
    bound, ported = pair

    bound_result, ported_result = bound.reverse(), ported.reverse()

    assert bound_result == ported_result
    assert are_bound_ported_vectors_equal(bound, ported)
