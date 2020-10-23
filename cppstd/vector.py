from typing import (Generic,
                    Iterable,
                    List,
                    Optional,
                    overload)

from reprit.base import (generate_repr,
                         seekers)

from .core.tokenization import (Token,
                                Tokenizer)
from .hints import Value


class vector(Generic[Value]):
    class iterator(Generic[Value]):
        __slots__ = '_index', '_values', '_token'

        def __init__(self, index: int, values: List[Value],
                     token: Token) -> None:
            self._index = index
            self._values = values
            self._token = token

        def __add__(self, offset: int) -> 'vector.iterator[Value]':
            return vector.iterator(self._move_index(offset), self._values,
                                   self._token)

        def __eq__(self, other: 'vector.iterator[Value]') -> bool:
            return (self._to_validated_values() is other._to_validated_values()
                    and self._index == other._index
                    if isinstance(other, vector.iterator)
                    else NotImplemented)

        def __iadd__(self, offset: int) -> 'vector.iterator[Value]':
            self._index = self._move_index(offset)
            return self

        def __isub__(self, offset: int) -> 'vector.iterator[Value]':
            self._index = self._move_index(-offset)
            return self

        def __le__(self, other: 'vector.iterator[Value]') -> bool:
            return (self._to_validated_values() is other._to_validated_values()
                    and self._index <= other._index
                    if isinstance(other, vector.iterator)
                    else NotImplemented)

        def __lt__(self, other: 'vector.iterator[Value]') -> bool:
            return (self._to_validated_values() is other._to_validated_values()
                    and self._index < other._index
                    if isinstance(other, vector.iterator)
                    else NotImplemented)

        def __sub__(self, offset: int) -> 'vector.iterator[Value]':
            return vector.iterator(self._move_index(-offset), self._values,
                                   self._token)

        def _move_index(self, offset: int) -> int:
            size = len(self._to_validated_values())
            min_offset, max_offset = -self._index, size - self._index
            if offset < min_offset or offset > max_offset:
                raise RuntimeError('Advancing of iterators out-of-bound '
                                   'is undefined: '
                                   'offset should be '
                                   'in range({min_offset}, {max_offset}), '
                                   'but found {offset}.'
                                   .format(min_offset=min_offset,
                                           max_offset=max_offset + 1,
                                           offset=offset)
                                   if self._index != size
                                   else 'Advancing of placeholder iterators '
                                        'is undefined.')
            return self._index + offset

        def _to_validated_values(self) -> List[Value]:
            self._validate()
            return self._values

        def _validate(self) -> None:
            if self._token.expired:
                raise RuntimeError('Iterator is invalidated.')

    class reverse_iterator(Generic[Value]):
        __slots__ = '_index', '_values', '_token'

        def __init__(self, index: int, values: List[Value],
                     token: Token) -> None:
            self._index = index
            self._values = values
            self._token = token

        def __add__(self, offset: int) -> 'vector.reverse_iterator[Value]':
            return vector.reverse_iterator(self._move_index(offset),
                                           self._values, self._token)

        def __eq__(self, other: 'vector.reverse_iterator[Value]') -> bool:
            return (self._to_validated_values() is other._to_validated_values()
                    and self._index == other._index
                    if isinstance(other, vector.reverse_iterator)
                    else NotImplemented)

        def __iadd__(self, offset: int) -> 'vector.reverse_iterator[Value]':
            self._index = self._move_index(offset)
            return self

        def __isub__(self, offset: int) -> 'vector.reverse_iterator[Value]':
            self._index = self._move_index(-offset)
            return self

        def __le__(self, other: 'vector.reverse_iterator[Value]') -> bool:
            return (self._to_validated_values() is other._to_validated_values()
                    and self._index <= other._index
                    if isinstance(other, vector.reverse_iterator)
                    else NotImplemented)

        def __lt__(self, other: 'vector.reverse_iterator[Value]') -> bool:
            return (self._to_validated_values() is other._to_validated_values()
                    and self._index < other._index
                    if isinstance(other, vector.reverse_iterator)
                    else NotImplemented)

        def __sub__(self, offset: int) -> 'vector.reverse_iterator[Value]':
            return vector.reverse_iterator(self._move_index(-offset),
                                           self._values, self._token)

        def _move_index(self, offset: int) -> int:
            size = len(self._to_validated_values())
            min_offset, max_offset = -self._index, size - self._index
            if offset < min_offset or offset > max_offset:
                raise RuntimeError('Advancing of iterators out-of-bound '
                                   'is undefined: '
                                   'offset should be '
                                   'in range({min_offset}, {max_offset}), '
                                   'but found {offset}.'
                                   .format(min_offset=min_offset,
                                           max_offset=max_offset + 1,
                                           offset=offset)
                                   if self._index != size
                                   else 'Advancing of placeholder iterators '
                                        'is undefined.')
            return self._index + offset

        def _to_validated_values(self) -> List[Value]:
            self._validate()
            return self._values

        def _validate(self) -> None:
            if self._token.expired:
                raise RuntimeError('Iterator is invalidated.')

    __slots__ = '_values', '_tokenizer'

    def __init__(self, *values: Value) -> None:
        self._values = list(values)  # type: List[Value]
        self._tokenizer = Tokenizer()

    __repr__ = generate_repr(__init__,
                             field_seeker=seekers.complex_)

    def __eq__(self, other: 'vector') -> bool:
        return (self._values == other._values
                if isinstance(other, vector)
                else NotImplemented)

    def __getitem__(self, index: int) -> Value:
        if isinstance(index, int):
            return self._values[index]
        else:
            raise TypeError('Vector indices must be integers, found: {}.'
                            .format(type(index)))

    def __le__(self, other: 'vector') -> bool:
        return (self._values <= other._values
                if isinstance(other, vector)
                else NotImplemented)

    def __lt__(self, other: 'vector') -> bool:
        return (self._values < other._values
                if isinstance(other, vector)
                else NotImplemented)

    @overload
    def __setitem__(self, item: int, value: Value) -> None:
        """Sets element by given index to given value."""

    @overload
    def __setitem__(self, item: slice, values: Iterable[Value]) -> None:
        """Sets subvector by given slice to given values."""

    def __setitem__(self, item, value):
        if isinstance(item, slice):
            start, stop, step = item.indices(len(self._values))
            value = list(value)
            if stop > start or step == 1 and value:
                self._tokenizer.reset()
        self._values[item] = value

    def begin(self) -> 'iterator[Value]':
        return vector.iterator(0, self._values, self._tokenizer.create())

    def clear(self) -> None:
        self._tokenizer.reset()
        self._values.clear()

    def end(self) -> 'iterator[Value]':
        return vector.iterator(self.size(), self._values,
                               self._tokenizer.create())

    def insert(self, index: int, value: Value) -> None:
        self._tokenizer.reset()
        self._values.insert(index, value)

    def pop_back(self) -> None:
        self._tokenizer.reset()
        del self._values[-1]

    def empty(self) -> bool:
        return not self._values

    def size(self) -> int:
        return len(self._values)

    def push_back(self, value: Value) -> None:
        self._tokenizer.reset()
        self._values.append(value)

    def rbegin(self) -> 'reverse_iterator[Value]':
        return vector.reverse_iterator(0, self._values,
                                       self._tokenizer.create())

    def rend(self) -> 'reverse_iterator[Value]':
        return vector.reverse_iterator(self.size(), self._values,
                                       self._tokenizer.create())

    def resize(self, size: int, value: Optional[Value] = None) -> None:
        if size < 0:
            raise ValueError('Size should be positive, but found {}.'
                             .format(size))
        self._tokenizer.reset()
        self._values = (self._values
                        + [value] * (size - len(self._values)))[:size]
