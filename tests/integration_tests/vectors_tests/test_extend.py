from typing import (Any,
                    List)

from hypothesis import given

from tests.utils import (BoundPortedVectorsPair,
                         are_bound_ported_vectors_equal)
from . import strategies


@given(strategies.non_empty_vectors_pairs, strategies.objects_lists)
def test_basic(pair: BoundPortedVectorsPair, values: List[Any]) -> None:
    bound, ported = pair

    bound_result, ported_result = bound.extend(values), ported.extend(values)

    assert bound_result is ported_result is None
    assert are_bound_ported_vectors_equal(bound, ported)
