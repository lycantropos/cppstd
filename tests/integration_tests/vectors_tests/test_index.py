from typing import (Any,
                    Tuple)

import pytest
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


@given(strategies.vectors_pairs_with_non_their_elements)
def test_defaults_missing(pair_with_value: Tuple[BoundPortedVectorsPair, Any]
                          ) -> None:
    (bound, ported), value = pair_with_value

    with pytest.raises(ValueError):
        bound.index(value)
    with pytest.raises(ValueError):
        ported.index(value)


@given(strategies.vectors_pairs_with_starts_stops_and_non_their_elements)
def test_full_missing(pair_with_value
                      : Tuple[BoundPortedVectorsPair, int, int, Any]) -> None:
    (bound, ported), start, stop, value = pair_with_value

    with pytest.raises(ValueError):
        bound.index(value, start, stop)
    with pytest.raises(ValueError):
        ported.index(value, start, stop)
