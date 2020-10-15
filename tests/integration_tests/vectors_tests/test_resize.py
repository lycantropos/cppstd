from typing import Any

import pytest
from hypothesis import given

from tests.utils import (BoundPortedVectorsPair,
                         are_bound_ported_vectors_equal)
from . import strategies


@given(strategies.non_empty_vectors_pairs, strategies.sizes)
def test_defaults(pair: BoundPortedVectorsPair, size: int) -> None:
    bound, ported = pair

    bound_result, ported_result = bound.resize(size), ported.resize(size)

    assert bound_result is ported_result is None
    assert are_bound_ported_vectors_equal(bound, ported)


@given(strategies.non_empty_vectors_pairs, strategies.sizes,
       strategies.objects)
def test_full(pair: BoundPortedVectorsPair, size: int, value: Any) -> None:
    bound, ported = pair

    bound_result, ported_result = (bound.resize(size, value),
                                   ported.resize(size, value))

    assert bound_result is ported_result is None
    assert are_bound_ported_vectors_equal(bound, ported)


@given(strategies.non_empty_vectors_pairs, strategies.invalid_sizes)
def test_defaults_invalid_size(pair: BoundPortedVectorsPair,
                               size: int) -> None:
    bound, ported = pair

    with pytest.raises(ValueError):
        bound.resize(size)
    with pytest.raises(ValueError):
        ported.resize(size)


@given(strategies.non_empty_vectors_pairs, strategies.invalid_sizes,
       strategies.objects)
def test_full_invalid_size(pair: BoundPortedVectorsPair,
                           size: int,
                           value: Any) -> None:
    bound, ported = pair

    with pytest.raises(ValueError):
        bound.resize(size, value)
    with pytest.raises(ValueError):
        ported.resize(size, value)
