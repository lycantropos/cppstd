from hypothesis import given

from cppstd.hints import (Key,
                          Value)
from tests.utils import (BoundPortedMapsPair,
                         are_bound_ported_maps_equal)
from . import strategies


@given(strategies.maps_pairs, strategies.keys, strategies.values)
def test_basic(pair: BoundPortedMapsPair, key: Key, value: Value) -> None:
    bound, ported = pair

    bound[key] = ported[key] = value

    assert are_bound_ported_maps_equal(bound, ported)
