from typing import (Any,
                    Tuple)

import pytest
from hypothesis import given

from cppstd.hints import Key
from tests.utils import (BoundPortedMapsPair,
                         are_bound_ported_maps_equal)
from . import strategies


@given(strategies.non_empty_maps_pairs_with_their_keys)
def test_basic(pair_with_key: Tuple[BoundPortedMapsPair, Key]) -> None:
    (bound, ported), key = pair_with_key

    del bound[key], ported[key]

    assert are_bound_ported_maps_equal(bound, ported)


@given(strategies.maps_pairs_with_non_their_keys)
def test_missing(pair_with_key: Tuple[BoundPortedMapsPair, Any]) -> None:
    (bound, ported), key = pair_with_key

    with pytest.raises(ValueError):
        del bound[key]
    with pytest.raises(ValueError):
        del ported[key]
