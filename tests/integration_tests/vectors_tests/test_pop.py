import pytest
from hypothesis import given

from tests.utils import (BoundPortedVectorsPair,
                         are_bound_ported_vectors_equal)
from . import strategies


@given(strategies.non_empty_vectors_pairs)
def test_basic(pair: BoundPortedVectorsPair) -> None:
    bound, ported = pair

    bound_result, ported_result = bound.pop(), ported.pop()

    assert bound_result == ported_result
    assert are_bound_ported_vectors_equal(bound, ported)


@given(strategies.empty_vectors_pairs)
def test_empty(pair: BoundPortedVectorsPair) -> None:
    bound, ported = pair

    with pytest.raises(IndexError):
        bound.pop()
    with pytest.raises(IndexError):
        ported.pop()
