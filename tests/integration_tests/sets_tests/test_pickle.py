from hypothesis import given

from tests.utils import (BoundPortedSetsPair,
                         are_bound_ported_sets_equal,
                         pickle_round_trip)
from . import strategies


@given(strategies.non_empty_sets_pairs)
def test_basic(pair: BoundPortedSetsPair) -> None:
    bound, ported = pair

    assert are_bound_ported_sets_equal(pickle_round_trip(bound),
                                       pickle_round_trip(ported))
