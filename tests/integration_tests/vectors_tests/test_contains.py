from typing import Any

from hypothesis import given

from tests.utils import (BoundPortedVectorsPair,
                         equivalence)
from . import strategies


@given(strategies.vectors_pairs, strategies.objects)
def test_basic(pair: BoundPortedVectorsPair, value: Any) -> None:
    bound, ported = pair

    assert equivalence(value in bound, value in ported)
