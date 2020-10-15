from typing import Any

from hypothesis import given

from tests.utils import BoundPortedVectorsPair
from . import strategies


@given(strategies.non_empty_vectors_pairs, strategies.objects)
def test_defaults(pair: BoundPortedVectorsPair, value: Any) -> None:
    bound, ported = pair

    bound_result, ported_result = bound.count(value), ported.count(value)

    assert bound_result == ported_result
