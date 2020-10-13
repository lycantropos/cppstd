from typing import (Generic,
                    List,
                    Union)

from reprit.base import (generate_repr,
                         seekers)

from .hints import Domain


class Vector(Generic[Domain]):
    __slots__ = '_values'

    def __init__(self, *values: Domain) -> None:
        self._values = list(values)  # type: List[Domain]

    __repr__ = generate_repr(__init__,
                             field_seeker=seekers.complex_)

    def __delitem__(self, item: Union[int, slice]) -> None:
        del self._values[item]

    def __eq__(self, other: 'Vector') -> bool:
        return (self._values == other._values
                if isinstance(other, Vector)
                else NotImplemented)

    def __len__(self) -> int:
        return len(self._values)

    def push_back(self, value: Domain) -> None:
        self._values.append(value)
