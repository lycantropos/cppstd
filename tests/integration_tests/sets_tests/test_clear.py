from hypothesis import given

from tests.utils import (BoundPortedSetsPair,
                         are_bound_ported_sets_equal)
from . import strategies


@given(strategies.sets_pairs)
def test_basic(pair: BoundPortedSetsPair) -> None:
    bound, ported = pair

    bound_result, ported_result = bound.clear(), ported.clear()

    assert bound_result is ported_result is None
    assert are_bound_ported_sets_equal(bound, ported)
