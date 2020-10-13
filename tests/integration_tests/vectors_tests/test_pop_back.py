import pytest
from hypothesis import given

from tests.utils import (BoundPortedVectorsPair,
                         are_bound_ported_vectors_equal)
from . import strategies


@given(strategies.non_empty_vectors_pairs)
def test_basic(pair: BoundPortedVectorsPair) -> None:
    bound, ported = pair

    bound_result, ported_result = bound.pop_back(), ported.pop_back()

    assert bound_result is ported_result is None
    assert are_bound_ported_vectors_equal(bound, ported)


@given(strategies.empty_vectors_pairs)
def test_empty(pair: BoundPortedVectorsPair) -> None:
    bound, ported = pair

    with pytest.raises(IndexError):
        bound.pop_back()
    with pytest.raises(IndexError):
        ported.pop_back()
