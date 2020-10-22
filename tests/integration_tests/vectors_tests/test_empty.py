from hypothesis import given

from tests.utils import (BoundPortedVectorsPair,
                         equivalence)
from . import strategies


@given(strategies.vectors_pairs)
def test_basic(pair: BoundPortedVectorsPair) -> None:
    bound, ported = pair

    assert equivalence(bound.empty(), ported.empty())
