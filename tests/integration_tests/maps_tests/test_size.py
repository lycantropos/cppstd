from hypothesis import given

from tests.utils import BoundPortedMapsPair
from . import strategies


@given(strategies.maps_pairs)
def test_basic(pair: BoundPortedMapsPair) -> None:
    bound, ported = pair

    assert bound.size() == ported.size()
