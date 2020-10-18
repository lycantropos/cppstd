from typing import (Any,
                    List)

from hypothesis import given

from tests.utils import PortedSet
from . import strategies


@given(strategies.objects_lists)
def test_properties(objects: List[Any]) -> None:
    result = PortedSet(*objects)

    unique_objects = frozenset(objects)
    assert len(result) == len(unique_objects)
    assert list(result) == sorted(unique_objects)
