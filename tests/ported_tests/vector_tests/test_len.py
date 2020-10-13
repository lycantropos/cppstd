from hypothesis import given

from tests.utils import (PortedVector,
                         equivalence)
from . import strategies


@given(strategies.vectors)
def test_sign(vector: PortedVector) -> None:
    result = len(vector)

    assert result >= 0


@given(strategies.vectors)
def test_connection_with_bool(vector: PortedVector) -> None:
    result = len(vector)

    assert equivalence(bool(result), bool(vector))
