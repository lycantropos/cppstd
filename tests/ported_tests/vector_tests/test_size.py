from hypothesis import given

from tests.utils import (PortedVector,
                         equivalence)
from . import strategies


@given(strategies.vectors)
def test_sign(vector: PortedVector) -> None:
    result = vector.size()

    assert result >= 0


@given(strategies.vectors)
def test_connection_with_bool(vector: PortedVector) -> None:
    result = vector.size()

    assert equivalence(bool(result), not vector.empty())
