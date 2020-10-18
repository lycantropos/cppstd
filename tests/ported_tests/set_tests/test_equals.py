from hypothesis import given

from tests.utils import (PortedSet,
                         implication)
from . import strategies


@given(strategies.sets)
def test_reflexivity(set: PortedSet) -> None:
    assert set == set


@given(strategies.sets, strategies.sets)
def test_symmetry(left_set: PortedSet, right_set: PortedSet) -> None:
    assert implication(left_set == right_set,
                       right_set == left_set)


@given(strategies.sets, strategies.sets, strategies.sets)
def test_transitivity(left_set: PortedSet,
                      mid_set: PortedSet,
                      right_set: PortedSet) -> None:
    assert implication(left_set == mid_set == right_set,
                       left_set == right_set)
