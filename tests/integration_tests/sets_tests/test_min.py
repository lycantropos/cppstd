import pytest
from hypothesis import given

from tests.utils import BoundPortedSetsPair
from . import strategies


@given(strategies.non_empty_sets_pairs)
def test_basic(pair: BoundPortedSetsPair) -> None:
    bound, ported = pair

    assert bound.min() == ported.min()


@given(strategies.empty_sets_pairs)
def test_empty(pair: BoundPortedSetsPair) -> None:
    bound, ported = pair

    with pytest.raises(ValueError):
        bound.min()
    with pytest.raises(ValueError):
        ported.min()
