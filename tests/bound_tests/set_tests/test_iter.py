from hypothesis import given

from tests.utils import (BoundSet,
                         capacity,
                         pairwise)
from . import strategies


@given(strategies.sets)
def test_idempotence(set: BoundSet) -> None:
    result = iter(set)

    assert iter(result) is result


@given(strategies.sets)
def test_capacity(set: BoundSet) -> None:
    result = iter(set)

    assert capacity(result) == len(set)


@given(strategies.sets)
def test_subsetness(set: BoundSet) -> None:
    result = iter(set)

    assert all(element in set for element in result)


@given(strategies.sets)
def test_order(set: BoundSet) -> None:
    result = iter(set)

    assert all(element < next_element
               for element, next_element in pairwise(result))
