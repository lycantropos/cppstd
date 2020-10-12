from hypothesis import given

from tests.utils import BoundVector
from . import strategies


@given(strategies.vectors)
def test_elements(vector: BoundVector) -> None:
    assert all(element in vector for element in vector)
