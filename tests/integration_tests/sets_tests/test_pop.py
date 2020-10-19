import pytest
from hypothesis import given

from tests.utils import (BoundPortedSetsPair,
                         are_bound_ported_sets_equal)
from . import strategies


@given(strategies.non_empty_sets_pairs)
def test_basic(pair: BoundPortedSetsPair) -> None:
    bound, ported = pair

    bound_result, ported_result = bound.pop(), ported.pop()

    assert bound_result == ported_result
    assert are_bound_ported_sets_equal(bound, ported)


@given(strategies.empty_sets_pairs)
def test_empty(pair: BoundPortedSetsPair) -> None:
    bound, ported = pair

    with pytest.raises(ValueError):
        bound.pop()
    with pytest.raises(ValueError):
        ported.pop()
