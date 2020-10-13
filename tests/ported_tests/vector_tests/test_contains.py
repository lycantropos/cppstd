from hypothesis import given

from tests.utils import PortedVector
from . import strategies


@given(strategies.vectors)
def test_elements(vector: PortedVector) -> None:
    assert all(element in vector for element in vector)
