from hypothesis import given

from tests.utils import (PortedSet,
                         equivalence)
from . import strategies


@given(strategies.sets)
def test_sign(set: PortedSet) -> None:
    result = set.size()

    assert result >= 0


@given(strategies.sets)
def test_connection_with_bool(set: PortedSet) -> None:
    result = set.size()

    assert equivalence(bool(result), not set.empty())
