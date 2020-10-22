from hypothesis import given

from tests.utils import BoundPortedSetsPair
from . import strategies


@given(strategies.sets_pairs)
def test_basic(pair: BoundPortedSetsPair) -> None:
    bound, ported = pair

    assert bound.size() == ported.size()
