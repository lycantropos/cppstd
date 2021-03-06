from hypothesis import given

from tests.utils import (BoundPortedMapsPair,
                         are_bound_ported_map_iterators_equal)
from . import strategies


@given(strategies.maps_pairs)
def test_basic(pair: BoundPortedMapsPair) -> None:
    bound, ported = pair

    bound_result, ported_result = bound.cbegin(), ported.cbegin()

    assert are_bound_ported_map_iterators_equal(bound_result, bound.cend(),
                                                ported_result, ported.cend())
