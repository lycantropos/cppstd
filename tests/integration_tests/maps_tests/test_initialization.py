from typing import List

from hypothesis import given

from cppstd.hints import Item
from tests.utils import (BoundMap,
                         PortedMap,
                         are_bound_ported_maps_equal)
from . import strategies


@given(strategies.items_lists)
def test_basic(items: List[Item]) -> None:
    bound, ported = BoundMap(*items), PortedMap(*items)

    assert are_bound_ported_maps_equal(bound, ported)
