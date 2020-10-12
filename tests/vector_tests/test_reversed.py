from hypothesis import given

from tests.utils import (BoundVector,
                         capacity)
from . import strategies


@given(strategies.vectors)
def test_capacity(vector: BoundVector) -> None:
    result = reversed(vector)

    assert capacity(result) == len(vector)


@given(strategies.vectors)
def test_elements(vector: BoundVector) -> None:
    result = reversed(vector)

    assert all(element is vector[-index]
               for index, element in enumerate(result,
                                               start=1))
