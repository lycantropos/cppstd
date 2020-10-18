from hypothesis import given

from tests.utils import (PortedSet,
                         equivalence)
from . import strategies


@given(strategies.sets)
def test_sign(set: PortedSet) -> None:
    result = len(set)

    assert result >= 0


@given(strategies.sets)
def test_connection_with_bool(set: PortedSet) -> None:
    result = len(set)

    assert equivalence(bool(result), bool(set))
