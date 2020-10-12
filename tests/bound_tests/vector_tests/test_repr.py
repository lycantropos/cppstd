import sys

from hypothesis import given

from tests.utils import BoundVector
from . import strategies


@given(strategies.vectors)
def test_basic(vector: BoundVector) -> None:
    result = repr(vector)

    assert result.startswith(BoundVector.__module__)
    assert BoundVector.__qualname__ in result


@given(strategies.vectors)
def test_round_trip(vector: BoundVector) -> None:
    result = repr(vector)

    assert eval(result, sys.modules) == vector
