from typing import (Any,
                    List)

from hypothesis import given

from tests.utils import (BoundVector,
                         PortedVector,
                         are_bound_ported_vectors_equal)
from . import strategies


@given(strategies.objects_lists)
def test_basic(objects: List[Any]) -> None:
    bound, ported = BoundVector(*objects), PortedVector(*objects)

    assert are_bound_ported_vectors_equal(bound, ported)
