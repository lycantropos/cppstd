from hypothesis import given

from tests.utils import (BoundSet,
                         implication)
from . import strategies


@given(strategies.sets)
def test_reflexivity(set: BoundSet) -> None:
    assert set == set


@given(strategies.sets, strategies.sets)
def test_symmetry(left_set: BoundSet, right_set: BoundSet) -> None:
    assert implication(left_set == right_set,
                       right_set == left_set)


@given(strategies.sets, strategies.sets, strategies.sets)
def test_transitivity(left_set: BoundSet,
                      mid_set: BoundSet,
                      right_set: BoundSet) -> None:
    assert implication(left_set == mid_set == right_set,
                       left_set == right_set)
