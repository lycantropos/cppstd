from hypothesis import given

from tests.utils import (BoundPortedMapsPair,
                         are_bound_ported_maps_equal)
from . import strategies


@given(strategies.maps_pairs)
def test_basic(pair: BoundPortedMapsPair) -> None:
    bound, ported = pair

    bound_result, ported_result = bound.clear(), ported.clear()

    assert bound_result is ported_result is None
    assert are_bound_ported_maps_equal(bound, ported)
