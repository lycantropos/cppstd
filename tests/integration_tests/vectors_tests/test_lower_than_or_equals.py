from hypothesis import given

from tests.utils import (BoundPortedVectorsPair,
                         equivalence)
from . import strategies


@given(strategies.vectors_pairs, strategies.vectors_pairs)
def test_basic(first_pair: BoundPortedVectorsPair,
               second_pair: BoundPortedVectorsPair) -> None:
    first_bound, first_ported = first_pair
    second_bound, second_ported = second_pair

    assert equivalence(first_bound <= second_bound,
                       first_ported <= second_ported)
