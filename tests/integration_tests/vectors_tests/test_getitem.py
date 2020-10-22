from typing import Tuple

import pytest
from hypothesis import given

from tests.utils import BoundPortedVectorsPair
from . import strategies


@given(strategies.non_empty_vectors_pairs_with_indices)
def test_index(pair_with_index: Tuple[BoundPortedVectorsPair, int]) -> None:
    (bound, ported), index = pair_with_index

    bound_result, ported_result = bound[index], ported[index]

    assert bound_result == ported_result


@given(strategies.vectors_pairs_with_invalid_indices)
def test_invalid_index(pair_with_index: Tuple[BoundPortedVectorsPair, int]
                       ) -> None:
    (bound, ported), index = pair_with_index

    with pytest.raises(IndexError):
        bound[index]
    with pytest.raises(IndexError):
        ported[index]
