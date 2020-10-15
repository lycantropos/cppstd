from typing import (Any,
                    Tuple)

import pytest
from hypothesis import given

from tests.utils import (BoundPortedVectorsPair,
                         are_bound_ported_vectors_equal)
from . import strategies


@given(strategies.non_empty_vectors_pairs_with_their_elements)
def test_basic(pair_with_value: Tuple[BoundPortedVectorsPair, Any]) -> None:
    (bound, ported), value = pair_with_value

    bound_result, ported_result = bound.remove(value), ported.remove(value)

    assert bound_result is ported_result is None
    assert are_bound_ported_vectors_equal(bound, ported)


@given(strategies.vectors_pairs_with_non_their_elements)
def test_missing(pair_with_value: Tuple[BoundPortedVectorsPair, Any]) -> None:
    (bound, ported), value = pair_with_value

    with pytest.raises(ValueError):
        bound.remove(value)
    with pytest.raises(ValueError):
        ported.remove(value)