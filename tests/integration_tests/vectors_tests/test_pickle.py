from hypothesis import given

from tests.utils import (BoundPortedVectorsPair,
                         are_bound_ported_vectors_equal,
                         pickle_round_trip)
from . import strategies


@given(strategies.non_empty_vectors_pairs)
def test_basic(pair: BoundPortedVectorsPair) -> None:
    bound, ported = pair

    assert are_bound_ported_vectors_equal(pickle_round_trip(bound),
                                          pickle_round_trip(ported))
