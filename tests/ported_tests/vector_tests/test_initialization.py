from typing import (Any,
                    List)

from hypothesis import given

from tests.utils import PortedVector
from . import strategies


@given(strategies.objects_lists)
def test_properties(objects: List[Any]) -> None:
    result = PortedVector(*objects)

    assert len(result) == len(objects)
    assert list(result) == objects
