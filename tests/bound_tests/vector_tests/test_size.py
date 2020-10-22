from hypothesis import given

from tests.utils import (BoundVector,
                         equivalence)
from . import strategies


@given(strategies.vectors)
def test_sign(vector: BoundVector) -> None:
    result = vector.size()

    assert result >= 0


@given(strategies.vectors)
def test_connection_with_bool(vector: BoundVector) -> None:
    result = vector.size()

    assert equivalence(bool(result), not vector.empty())
