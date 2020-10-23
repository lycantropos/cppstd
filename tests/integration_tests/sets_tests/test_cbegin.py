from hypothesis import given

from tests.utils import (BoundPortedSetsPair,
                         are_bound_ported_set_iterators_equal)
from . import strategies


@given(strategies.sets_pairs)
def test_basic(pair: BoundPortedSetsPair) -> None:
    bound, ported = pair

    bound_result, ported_result = bound.cbegin(), ported.cbegin()

    assert are_bound_ported_set_iterators_equal(bound_result, bound.cend(),
                                                ported_result, ported.cend())
