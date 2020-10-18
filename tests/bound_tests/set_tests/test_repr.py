import sys

from hypothesis import given

from tests.utils import BoundSet
from . import strategies


@given(strategies.sets)
def test_basic(set: BoundSet) -> None:
    result = repr(set)

    assert result.startswith(BoundSet.__module__)
    assert BoundSet.__qualname__ in result


@given(strategies.sets)
def test_round_trip(set: BoundSet) -> None:
    result = repr(set)

    assert eval(result, sys.modules) == set
