from hypothesis import given

from tests.utils import BoundSet
from . import strategies


@given(strategies.sets)
def test_elements(set: BoundSet) -> None:
    assert all(element in set for element in set)
