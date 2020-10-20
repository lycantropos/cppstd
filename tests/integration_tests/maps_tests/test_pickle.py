from hypothesis import given

from tests.utils import (BoundPortedMapsPair,
                         are_bound_ported_maps_equal,
                         pickle_round_trip)
from . import strategies


@given(strategies.non_empty_maps_pairs)
def test_basic(pair: BoundPortedMapsPair) -> None:
    bound, ported = pair

    assert are_bound_ported_maps_equal(pickle_round_trip(bound),
                                       pickle_round_trip(ported))
