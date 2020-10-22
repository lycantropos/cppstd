from typing import (Any,
                    Generic)

from reprit.base import (generate_repr,
                         seekers)

from .core import red_black
from .core.tokenization import Tokenizer
from .core.utils import (lexicographically_lower_than,
                         lexicographically_lower_than_or_equal)
from .hints import Value


class set(Generic[Value]):
    class iterator(red_black.TreeIterator, Generic[Value]):
        pass

    class reverse_iterator(red_black.TreeReverseIterator, Generic[Value]):
        pass

    __slots__ = '_values', '_tokenizer'

    def __init__(self, *_values: Value) -> None:
        self._values = red_black.set_(*_values)
        self._tokenizer = Tokenizer()

    __repr__ = generate_repr(__init__,
                             field_seeker=seekers.complex_)

    def __eq__(self, other: 'set[Value]') -> bool:
        return (self._values == other._values
                if isinstance(other, set)
                else NotImplemented)

    def __le__(self, other: 'set[Value]',
               *,
               _sentinel: Any = object()) -> bool:
        return (lexicographically_lower_than_or_equal(self._values,
                                                      other._values)
                if isinstance(other, set)
                else NotImplemented)

    def __lt__(self, other: 'set[Value]') -> bool:
        return (lexicographically_lower_than(self._values, other._values)
                if isinstance(other, set)
                else NotImplemented)

    def begin(self) -> iterator[Value]:
        return self.iterator(0, self._values.tree.min(),
                             self._values.tree, self._tokenizer.create())

    def clear(self) -> None:
        self._tokenizer.reset()
        self._values.clear()

    def empty(self) -> bool:
        return not self._values

    def end(self) -> iterator[Value]:
        return self.iterator(len(self._values), red_black.NIL,
                             self._values.tree, self._tokenizer.create())

    def rbegin(self) -> reverse_iterator[Value]:
        return self.reverse_iterator(0, self._values.tree.max(),
                                     self._values.tree,
                                     self._tokenizer.create())

    def rend(self) -> reverse_iterator[Value]:
        return self.reverse_iterator(len(self._values), red_black.NIL,
                                     self._values.tree,
                                     self._tokenizer.create())

    def size(self) -> int:
        return len(self._values)
