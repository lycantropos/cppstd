from functools import partial
from itertools import zip_longest
from typing import (Any,
                    Iterable)


def lexicographically_compare(left: Iterable[Any],
                              right: Iterable[Any],
                              *,
                              equality_value: bool,
                              _sentinel: Any = object()) -> bool:
    return next((value is _sentinel
                 or other_value is not _sentinel and value < other_value
                 for value, other_value in zip_longest(left, right,
                                                       fillvalue=_sentinel)
                 if value != other_value),
                equality_value)


lexicographically_lower_than = partial(lexicographically_compare,
                                       equality_value=False)
lexicographically_lower_than_or_equal = partial(lexicographically_compare,
                                                equality_value=True)
