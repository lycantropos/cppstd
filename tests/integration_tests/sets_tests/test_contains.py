from typing import Any

from hypothesis import given

from tests.utils import (BoundPortedSetsPair,
                         equivalence)
from . import strategies


@given(strategies.sets_pairs, strategies.objects)
def test_basic(pair: BoundPortedSetsPair, value: Any) -> None:
    bound, ported = pair

    assert equivalence(value in bound, value in ported)
