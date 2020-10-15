from typing import (Any,
                    Tuple)

from hypothesis import given

from tests.utils import BoundPortedVectorsPair
from . import strategies


@given(strategies.non_empty_vectors_pairs_with_their_elements)
def test_defaults(pair_with_value: Tuple[BoundPortedVectorsPair, Any]) -> None:
    (bound, ported), value = pair_with_value

    bound_result, ported_result = bound.index(value), ported.index(value)

    assert bound_result == ported_result


@given(strategies.non_empty_vectors_pairs_with_starts_stops_and_their_elements)
def test_full(pair_with_start_stop_and_value
              : Tuple[BoundPortedVectorsPair, int, int, Any]) -> None:
    (bound, ported), start, stop, value = pair_with_start_stop_and_value

    bound_result, ported_result = (bound.index(value, start, stop),
                                   ported.index(value, start, stop))

    assert bound_result == ported_result
