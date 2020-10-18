from typing import (Any,
                    List)

from hypothesis import given

from tests.utils import (BoundSet,
                         PortedSet,
                         are_bound_ported_sets_equal)
from . import strategies


@given(strategies.objects_lists)
def test_basic(objects: List[Any]) -> None:
    bound, ported = BoundSet(*objects), PortedSet(*objects)

    assert are_bound_ported_sets_equal(bound, ported)
