import sys
from collections import abc
from typing import (Generic,
                    Iterable,
                    Iterator,
                    List,
                    Optional,
                    Union,
                    overload)

from reprit.base import (generate_repr,
                         seekers)

from .hints import Domain


@abc.MutableSequence.register
class Vector(Generic[Domain]):
    __slots__ = '_values'

    def __init__(self, *values: Domain) -> None:
        self._values = list(values)  # type: List[Domain]

    __repr__ = generate_repr(__init__,
                             field_seeker=seekers.complex_)

    def __bool__(self) -> bool:
        return bool(self._values)

    def __contains__(self, value: Domain) -> bool:
        return value in self._values

    def __delitem__(self, item: Union[int, slice]) -> None:
        del self._values[item]

    def __eq__(self, other: 'Vector') -> bool:
        return (self._values == other._values
                if isinstance(other, Vector)
                else NotImplemented)

    @overload
    def __getitem__(self, item: int) -> Domain:
        """Returns element by given index."""

    @overload
    def __getitem__(self, item: slice) -> 'Vector[Domain]':
        """Returns subvector by given slice."""

    def __getitem__(self, item):
        return (self._values[item]
                if isinstance(item, int)
                else Vector(*self._values[item]))

    def __iter__(self) -> 'VectorForwardIterator[Domain]':
        return VectorForwardIterator(0, self)

    def __len__(self) -> int:
        return len(self._values)

    def __le__(self, other: 'Vector') -> bool:
        return (self._values <= other._values
                if isinstance(other, Vector)
                else NotImplemented)

    def __lt__(self, other: 'Vector') -> bool:
        return (self._values < other._values
                if isinstance(other, Vector)
                else NotImplemented)

    def __reversed__(self) -> 'VectorBackwardIterator[Domain]':
        return VectorBackwardIterator(0, self)

    @overload
    def __setitem__(self, item: int, value: Domain) -> None:
        """Sets element by given index to given value."""

    @overload
    def __setitem__(self, item: slice, values: Iterable[Domain]) -> None:
        """Sets subvector by given slice to given values."""

    def __setitem__(self, item, value):
        self._values[item] = value

    def append(self, value: Domain) -> None:
        self._values.append(value)

    def begin(self) -> 'VectorForwardIterator[Domain]':
        return VectorForwardIterator(0, self)

    def clear(self) -> None:
        self._values.clear()

    def count(self, value: Domain) -> int:
        return self._values.count(value)

    def end(self) -> 'VectorForwardIterator[Domain]':
        return VectorForwardIterator(len(self), self)

    def extend(self, values: Iterable[Domain]) -> None:
        self._values.extend(values)

    def index(self,
              value: Domain,
              start: int = 0,
              stop: int = sys.maxsize) -> int:
        return self._values.index(value, start, stop)

    def insert(self, index: int, value: Domain) -> None:
        self._values.insert(index, value)

    def pop(self, index: int = -1) -> None:
        return self._values.pop(index)

    def pop_back(self) -> None:
        del self._values[-1]

    push_back = append

    def rbegin(self) -> 'VectorBackwardIterator[Domain]':
        return VectorBackwardIterator(0, self)

    def remove(self, value: Domain) -> None:
        self._values.remove(value)

    def rend(self) -> 'VectorBackwardIterator[Domain]':
        return VectorBackwardIterator(len(self), self)

    def resize(self, size: int, value: Optional[Domain] = None) -> None:
        if size < 0:
            raise ValueError('Size should be positive, but found {}.'
                             .format(size))
        self._values = (self._values
                        + [value] * (size - len(self._values)))[:size]

    def reverse(self) -> None:
        self._values.reverse()


class VectorBackwardIterator(Iterator[Domain]):
    __slots__ = '_index', '_vector'

    def __init__(self, index: int, vector: Vector) -> None:
        self._index = index
        self._vector = vector

    def __add__(self, offset: int) -> 'VectorBackwardIterator[Domain]':
        return VectorBackwardIterator(self._move_index(offset), self._vector)

    def __iadd__(self, offset: int) -> 'VectorBackwardIterator[Domain]':
        self._index = self._move_index(offset)
        return self

    def __isub__(self, offset: int) -> 'VectorBackwardIterator[Domain]':
        self._index = self._move_index(-offset)
        return self

    def __iter__(self) -> 'VectorBackwardIterator[Domain]':
        return self

    def __le__(self, other: 'VectorBackwardIterator[Domain]') -> bool:
        return (self._vector is other._vector and self._index <= other._index
                if isinstance(other, VectorBackwardIterator)
                else NotImplemented)

    def __lt__(self, other: 'VectorBackwardIterator[Domain]') -> bool:
        return (self._vector is other._vector and self._index < other._index
                if isinstance(other, VectorBackwardIterator)
                else NotImplemented)

    def __next__(self) -> Domain:
        try:
            result = self._vector[-self._index - 1]
        except IndexError:
            raise StopIteration from None
        else:
            self._index += 1
            return result

    def __sub__(self, offset: int) -> 'VectorBackwardIterator[Domain]':
        return VectorBackwardIterator(self._move_index(-offset), self._vector)

    def _move_index(self, offset: int) -> int:
        size = len(self._vector)
        min_offset, max_offset = -self._index, size - self._index
        if offset < min_offset or offset > max_offset:
            raise ValueError('Offset should be '
                             'in range({min_offset}, {max_offset}), '
                             'but found {offset}.'
                             .format(min_offset=min_offset,
                                     max_offset=max_offset + 1,
                                     offset=offset)
                             if size
                             else 'Vector is empty.')
        return self._index + offset


class VectorForwardIterator(Iterator[Domain]):
    __slots__ = '_index', '_vector'

    def __init__(self, index: int, vector: Vector) -> None:
        self._index = index
        self._vector = vector

    def __add__(self, offset: int) -> 'VectorForwardIterator[Domain]':
        return VectorForwardIterator(self._move_index(offset), self._vector)

    def __iadd__(self, offset: int) -> 'VectorForwardIterator[Domain]':
        self._index = self._move_index(offset)
        return self

    def __isub__(self, offset: int) -> 'VectorForwardIterator[Domain]':
        self._index = self._move_index(-offset)
        return self

    def __iter__(self) -> 'VectorForwardIterator[Domain]':
        return self

    def __le__(self, other: 'VectorForwardIterator[Domain]') -> bool:
        return (self._vector is other._vector and self._index <= other._index
                if isinstance(other, VectorForwardIterator)
                else NotImplemented)

    def __lt__(self, other: 'VectorForwardIterator[Domain]') -> bool:
        return (self._vector is other._vector and self._index < other._index
                if isinstance(other, VectorForwardIterator)
                else NotImplemented)

    def __next__(self) -> Domain:
        try:
            result = self._vector[self._index]
        except IndexError:
            raise StopIteration from None
        else:
            self._index += 1
            return result

    def __sub__(self, offset: int) -> 'VectorForwardIterator[Domain]':
        return VectorForwardIterator(self._move_index(-offset), self._vector)

    def _move_index(self, offset: int) -> int:
        size = len(self._vector)
        min_offset, max_offset = -self._index, size - self._index
        if offset < min_offset or offset > max_offset:
            raise ValueError('Offset should be '
                             'in range({min_offset}, {max_offset}), '
                             'but found {offset}.'
                             .format(min_offset=min_offset,
                                     max_offset=max_offset + 1,
                                     offset=offset)
                             if size
                             else 'Vector is empty.')
        return self._index + offset
