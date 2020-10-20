from typing import Any

from hypothesis import given

from tests.utils import (BoundPortedMapsPair,
                         equivalence)
from . import strategies


@given(strategies.maps_pairs, strategies.keys)
def test_basic(pair: BoundPortedMapsPair, key: Any) -> None:
    bound, ported = pair

    assert equivalence(key in bound, key in ported)
