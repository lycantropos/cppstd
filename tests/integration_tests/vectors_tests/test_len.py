from hypothesis import given

from tests.utils import BoundPortedVectorsPair
from . import strategies


@given(strategies.vectors_pairs)
def test_basic(pair: BoundPortedVectorsPair) -> None:
    bound, ported = pair

    assert len(bound) == len(ported)
