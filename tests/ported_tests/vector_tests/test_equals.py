from hypothesis import given

from tests.utils import (PortedVector,
                         implication)
from . import strategies


@given(strategies.vectors)
def test_reflexivity(vector: PortedVector) -> None:
    assert vector == vector


@given(strategies.vectors, strategies.vectors)
def test_symmetry(left_vector: PortedVector,
                  right_vector: PortedVector) -> None:
    assert implication(left_vector == right_vector,
                       right_vector == left_vector)


@given(strategies.vectors, strategies.vectors,
       strategies.vectors)
def test_transitivity(left_vector: PortedVector,
                      mid_vector: PortedVector,
                      right_vector: PortedVector) -> None:
    assert implication(left_vector == mid_vector == right_vector,
                       left_vector == right_vector)
