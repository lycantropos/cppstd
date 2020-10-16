from typing import Generic

from dendroid import red_black
from reprit.base import (generate_repr,
                         seekers)

from .hints import Domain


class Set(Generic[Domain]):
    __slots__ = '_values',

    def __init__(self, *values: Domain) -> None:
        self._values = red_black.set_(*values)

    __repr__ = generate_repr(__init__,
                             field_seeker=seekers.complex_)

    def __bool__(self) -> bool:
        return bool(self._values)

    def __eq__(self, other: 'Set[Domain]') -> bool:
        return (self._values == other._values
                if isinstance(other, Set)
                else NotImplemented)
