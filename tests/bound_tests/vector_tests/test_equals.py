from hypothesis import given

from tests.utils import (BoundVector,
                         implication)
from . import strategies


@given(strategies.vectors)
def test_reflexivity(vector: BoundVector) -> None:
    assert vector == vector


@given(strategies.vectors, strategies.vectors)
def test_symmetry(left_vector: BoundVector, right_vector: BoundVector) -> None:
    assert implication(left_vector == right_vector,
                       right_vector == left_vector)


@given(strategies.vectors, strategies.vectors,
       strategies.vectors)
def test_transitivity(left_vector: BoundVector,
                      mid_vector: BoundVector,
                      right_vector: BoundVector) -> None:
    assert implication(left_vector == mid_vector == right_vector,
                       left_vector == right_vector)
