from collections import abc
from typing import Generic

from reprit.base import (generate_repr,
                         seekers)

from .core import red_black
from .core.hints import RawSet
from .core.tokenization import Tokenizer
from .hints import Value


class SetBackwardIterator(red_black.TreeBackwardIterator, Generic[Value]):
    def __next__(self) -> Value:
        return super().__next__().value


class SetForwardIterator(red_black.TreeForwardIterator, Generic[Value]):
    def __next__(self) -> Value:
        return super().__next__().value


@abc.MutableSet.register
class Set(Generic[Value]):
    __slots__ = '_values', '_tokenizer'

    def __init__(self, *values: Value) -> None:
        self._values = red_black.set_(*values)
        self._tokenizer = Tokenizer()

    __repr__ = generate_repr(__init__,
                             field_seeker=seekers.complex_)

    def __and__(self, other: 'Set[Value]') -> 'Set[Value]':
        return (self._from_raw(self._values & other._values)
                if isinstance(other, Set)
                else NotImplemented)

    def __bool__(self) -> bool:
        return bool(self._values)

    def __contains__(self, value: Value):
        return value in self._values

    def __eq__(self, other: 'Set[Value]') -> bool:
        return (self._values == other._values
                if isinstance(other, Set)
                else NotImplemented)

    def __iand__(self, other: 'Set[Value]') -> 'Set[Value]':
        if not isinstance(other, Set):
            return NotImplemented
        size = len(self._values)
        if len(other._values) != size:
            self._tokenizer.reset()
            self._values &= other._values
        else:
            common_values = self._values & other._values
            if len(common_values) != size:
                self._tokenizer.reset()
                self._values = common_values
        return self

    def __ior__(self, other: 'Set[Value]') -> 'Set[Value]':
        if not isinstance(other, Set):
            return NotImplemented
        extra_values = other._values - self._values
        if extra_values:
            self._tokenizer.reset()
            self._values |= extra_values
        return self

    def __isub__(self, other: 'Set[Value]') -> 'Set[Value]':
        if not isinstance(other, Set):
            return NotImplemented
        common_values = self._values & other._values
        if common_values:
            self._tokenizer.reset()
            self._values -= common_values
        return self

    def __iter__(self) -> SetForwardIterator[Value]:
        return SetForwardIterator(0, self._values.tree.min(),
                                  self._values.tree, self._tokenizer.create())

    def __ixor__(self, other: 'Set[Value]') -> 'Set[Value]':
        if not isinstance(other, Set):
            return NotImplemented
        if other:
            self._tokenizer.reset()
            self._values ^= other._values
        return self

    def __len__(self) -> int:
        return len(self._values)

    def __lt__(self, other: 'Set[Value]') -> bool:
        return (self._values < other._values
                if isinstance(other, Set)
                else NotImplemented)

    def __le__(self, other: 'Set[Value]') -> bool:
        return (self._values <= other._values
                if isinstance(other, Set)
                else NotImplemented)

    def __or__(self, other: 'Set[Value]') -> 'Set[Value]':
        return (self._from_raw(self._values | other._values)
                if isinstance(other, Set)
                else NotImplemented)

    def __reversed__(self) -> SetBackwardIterator[Value]:
        return SetBackwardIterator(0, self._values.tree.max(),
                                   self._values.tree, self._tokenizer.create())

    def __sub__(self, other: 'Set[Value]') -> 'Set[Value]':
        return (self._from_raw(self._values - other._values)
                if isinstance(other, Set)
                else NotImplemented)

    def __xor__(self, other: 'Set[Value]') -> 'Set[Value]':
        return (self._from_raw(self._values ^ other._values)
                if isinstance(other, Set)
                else NotImplemented)

    @classmethod
    def _from_raw(cls, raw: RawSet[Value]) -> 'Set[Value]':
        result = Set()
        result._values = raw
        return result

    def add(self, value: Value) -> None:
        if value not in self._values:
            self._tokenizer.reset()
            self._values.add(value)

    def begin(self) -> SetForwardIterator[Value]:
        return SetForwardIterator(0, self._values.tree.min(),
                                  self._values.tree, self._tokenizer.create())

    def clear(self) -> None:
        self._tokenizer.reset()
        self._values.clear()

    def discard(self, value: Value) -> None:
        node = self._values.tree.find(value)
        if node is not red_black.NIL:
            self._tokenizer.reset()
            self._values.tree.remove(node)

    def end(self) -> SetForwardIterator[Value]:
        return SetForwardIterator(len(self._values), red_black.NIL,
                                  self._values.tree, self._tokenizer.create())

    def isdisjoint(self, other: 'Set[Value]') -> bool:
        return self._values.isdisjoint(other._values)

    def max(self) -> Value:
        return self._values.max()

    def min(self) -> Value:
        return self._values.min()

    def pop(self) -> Value:
        if self._values:
            self._tokenizer.reset()
        return self._values.pop()

    def rbegin(self) -> SetBackwardIterator[Value]:
        return SetBackwardIterator(0, self._values.tree.max(),
                                   self._values.tree, self._tokenizer.create())

    def remove(self, value: Value) -> None:
        node = self._values.tree.find(value)
        if node is red_black.NIL:
            raise ValueError('{!r} is not found.'.format(value))
        else:
            self._tokenizer.reset()
            self._values.tree.remove(node)

    def rend(self) -> SetBackwardIterator[Value]:
        return SetBackwardIterator(len(self._values), red_black.NIL,
                                   self._values.tree, self._tokenizer.create())
