from typing import Any

from hypothesis import given

from tests.utils import (BoundPortedSetsPair,
                         are_bound_ported_sets_equal)
from . import strategies


@given(strategies.sets_pairs, strategies.objects)
def test_basic(pair: BoundPortedSetsPair, value: Any) -> None:
    bound, ported = pair

    bound_result, ported_result = bound.discard(value), ported.discard(value)

    assert bound_result is ported_result is None
    assert are_bound_ported_sets_equal(bound, ported)
