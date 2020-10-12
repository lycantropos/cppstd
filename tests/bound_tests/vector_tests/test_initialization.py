from typing import (Any,
                    List)

from hypothesis import given

from tests.utils import BoundVector
from . import strategies


@given(strategies.objects_lists)
def test_properties(objects: List[Any]) -> None:
    result = BoundVector(*objects)

    assert len(result) == len(objects)
    assert list(result) == objects
