from hypothesis import given

from tests.utils import (BoundPortedSetsPair,
                         equivalence)
from . import strategies


@given(strategies.sets_pairs)
def test_basic(pair: BoundPortedSetsPair) -> None:
    bound, ported = pair

    assert equivalence(bound.empty(), ported.empty())
