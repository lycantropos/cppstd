from hypothesis import given

from tests.utils import (PortedVector,
                         capacity)
from . import strategies


@given(strategies.vectors)
def test_idempotence(vector: PortedVector) -> None:
    result = iter(vector)

    assert iter(result) is result


@given(strategies.vectors)
def test_capacity(vector: PortedVector) -> None:
    result = iter(vector)

    assert capacity(result) == len(vector)


@given(strategies.vectors)
def test_elements(vector: PortedVector) -> None:
    result = iter(vector)

    assert all(element is vector[index]
               for index, element in enumerate(result))
