import weakref
from itertools import groupby
from typing import (List, Optional,
                    Sequence,
                    Tuple,
                    TypeVar)

from .hints import (Item,
                    Key,
                    Value)

Domain = TypeVar('Domain')


class AntisymmetricKeyIndex:
    __slots__ = 'key', 'index'

    def __init__(self, key_index: Tuple[Key, int]) -> None:
        self.key, self.index = key_index

    def __eq__(self, other: 'AntisymmetricKeyIndex') -> bool:
        return are_keys_equal(self.key, other.key)


def are_keys_equal(left: Key, right: Key) -> bool:
    return not (left < right or right < left)


def dereference_maybe(maybe_reference: Optional[weakref.ref]
                      ) -> Optional[Value]:
    return (maybe_reference
            if maybe_reference is None
            else maybe_reference())


def floor_log2(number: int) -> int:
    """
    Returns infimum of powers-of-two which are not greater than the number,
    equivalent of ``floor(log2(number))``.
    >>> floor_log2(1)
    0
    >>> floor_log2(2)
    1
    >>> floor_log2(3)
    1
    >>> floor_log2(4)
    2
    """
    return number.bit_length() - 1


def identity(value: Domain) -> Domain:
    return value


def maybe_weakref(object_: Optional[Value]
                  ) -> Optional[weakref.ReferenceType]:
    return (object_
            if object_ is None
            else weakref.ref(object_))


def to_unique_sorted_items(keys: Sequence[Key], values: Sequence[Value]
                           ) -> List[Item]:
    return [(index_key.key, values[-index_key.index])
            for index_key, _ in groupby(
                sorted([(key, -index) for index, key in enumerate(keys)]),
                key=AntisymmetricKeyIndex)]


def to_unique_sorted_values(values: List[Value]) -> List[Value]:
    values.sort()
    return [value for value, _ in groupby(values)]
