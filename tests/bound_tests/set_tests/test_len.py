from hypothesis import given

from tests.utils import (BoundSet,
                         equivalence)
from . import strategies


@given(strategies.sets)
def test_sign(set: BoundSet) -> None:
    result = len(set)

    assert result >= 0


@given(strategies.sets)
def test_connection_with_bool(set: BoundSet) -> None:
    result = len(set)

    assert equivalence(bool(result), bool(set))
