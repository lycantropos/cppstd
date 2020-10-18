from hypothesis import given

from tests.utils import PortedSet
from . import strategies


@given(strategies.sets)
def test_elements(set: PortedSet) -> None:
    assert all(element in set for element in set)
