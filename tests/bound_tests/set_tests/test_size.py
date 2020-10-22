from hypothesis import given

from tests.utils import (BoundSet,
                         equivalence)
from . import strategies


@given(strategies.sets)
def test_sign(set: BoundSet) -> None:
    result = set.size()

    assert result >= 0


@given(strategies.sets)
def test_connection_with_bool(set: BoundSet) -> None:
    result = set.size()

    assert equivalence(bool(result), not set.empty())
