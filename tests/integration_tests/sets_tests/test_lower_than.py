from hypothesis import given

from tests.utils import (BoundPortedSetsPair,
                         equivalence)
from . import strategies


@given(strategies.sets_pairs, strategies.sets_pairs)
def test_basic(first_pair: BoundPortedSetsPair,
               second_pair: BoundPortedSetsPair) -> None:
    first_bound, first_ported = first_pair
    second_bound, second_ported = second_pair

    assert equivalence(first_bound < second_bound,
                       first_ported < second_ported)
