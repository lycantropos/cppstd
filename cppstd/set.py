from collections import abc
from itertools import islice
from typing import (Generic,
                    Iterator,
                    Tuple,
                    Union)

from dendroid import red_black
from dendroid.hints import Set as RawSet
from reprit.base import (generate_repr,
                         seekers)

from .core.tokenization import (Token,
                                Tokenizer)
from .hints import Domain

AnyNode = Union[red_black.NIL, red_black.Node]


@abc.MutableSet.register
class Set(Generic[Domain]):
    __slots__ = '_values', '_tokenizer'

    def __init__(self, *values: Domain) -> None:
        self._values = red_black.set_(*values)
        self._tokenizer = Tokenizer()

    __repr__ = generate_repr(__init__,
                             field_seeker=seekers.complex_)

    def __and__(self, other: 'Set[Domain]') -> 'Set[Domain]':
        return (self._from_raw(self._values & other._values)
                if isinstance(other, Set)
                else NotImplemented)

    def __bool__(self) -> bool:
        return bool(self._values)

    def __contains__(self, value: Domain):
        return value in self._values

    def __eq__(self, other: 'Set[Domain]') -> bool:
        return (self._values == other._values
                if isinstance(other, Set)
                else NotImplemented)

    def __iand__(self, other: 'Set[Domain]') -> 'Set[Domain]':
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

    def __ior__(self, other: 'Set[Domain]') -> 'Set[Domain]':
        if not isinstance(other, Set):
            return NotImplemented
        extra_values = other._values - self._values
        if extra_values:
            self._tokenizer.reset()
            self._values |= extra_values
        return self

    def __isub__(self, other: 'Set[Domain]') -> 'Set[Domain]':
        if not isinstance(other, Set):
            return NotImplemented
        common_values = self._values & other._values
        if common_values:
            self._tokenizer.reset()
            self._values -= common_values
        return self

    def __iter__(self) -> 'SetForwardIterator[Domain]':
        return SetForwardIterator(0, self._values.tree.min(),
                                  self._values.tree, self._tokenizer.create())

    def __ixor__(self, other: 'Set[Domain]') -> 'Set[Domain]':
        if not isinstance(other, Set):
            return NotImplemented
        if other:
            self._tokenizer.reset()
            self._values ^= other._values
        return self

    def __len__(self) -> int:
        return len(self._values)

    def __lt__(self, other: 'Set[Domain]') -> bool:
        return (self._values < other._values
                if isinstance(other, Set)
                else NotImplemented)

    def __le__(self, other: 'Set[Domain]') -> bool:
        return (self._values <= other._values
                if isinstance(other, Set)
                else NotImplemented)

    def __or__(self, other: 'Set[Domain]') -> 'Set[Domain]':
        return (self._from_raw(self._values | other._values)
                if isinstance(other, Set)
                else NotImplemented)

    def __reversed__(self) -> 'SetBackwardIterator[Domain]':
        return SetBackwardIterator(0, self._values.tree.max(),
                                   self._values.tree, self._tokenizer.create())

    def __sub__(self, other: 'Set[Domain]') -> 'Set[Domain]':
        return (self._from_raw(self._values - other._values)
                if isinstance(other, Set)
                else NotImplemented)

    def __xor__(self, other: 'Set[Domain]') -> 'Set[Domain]':
        return (self._from_raw(self._values ^ other._values)
                if isinstance(other, Set)
                else NotImplemented)

    @classmethod
    def _from_raw(cls, raw: RawSet[Domain]) -> 'Set[Domain]':
        result = Set()
        result._values = raw
        return result

    def add(self, value: Domain) -> None:
        if value not in self._values:
            self._tokenizer.reset()
            self._values.add(value)

    def begin(self) -> 'SetForwardIterator[Domain]':
        return SetForwardIterator(0, self._values.tree.min(),
                                  self._values.tree, self._tokenizer.create())

    def clear(self) -> None:
        self._tokenizer.reset()
        self._values.clear()

    def discard(self, value: Domain) -> None:
        node = self._values.tree.find(value)
        if node is not red_black.NIL:
            self._tokenizer.reset()
            self._values.tree.remove(node)

    def end(self) -> 'SetForwardIterator[Domain]':
        return SetForwardIterator(len(self._values), red_black.NIL,
                                  self._values.tree, self._tokenizer.create())

    def isdisjoint(self, other: 'Set[Domain]') -> bool:
        return self._values.isdisjoint(other._values)

    def max(self) -> Domain:
        return self._values.max()

    def min(self) -> Domain:
        return self._values.min()

    def pop(self) -> Domain:
        if self._values:
            self._tokenizer.reset()
        return self._values.pop()

    def rbegin(self) -> 'SetBackwardIterator[Domain]':
        return SetBackwardIterator(0, self._values.tree.max(),
                                   self._values.tree, self._tokenizer.create())

    def remove(self, value: Domain) -> None:
        node = self._values.tree.find(value)
        if node is red_black.NIL:
            raise ValueError('{!r} is not found.'.format(value))
        else:
            self._tokenizer.reset()
            self._values.tree.remove(node)

    def rend(self) -> 'SetBackwardIterator[Domain]':
        return SetBackwardIterator(len(self._values), red_black.NIL,
                                   self._values.tree, self._tokenizer.create())


class SetBackwardIterator(Iterator[Domain]):
    __slots__ = '_index', '_node', '_tree', '_token'

    def __init__(self,
                 index: int,
                 node: AnyNode,
                 tree: red_black.Tree[Domain, Domain],
                 token: Token) -> None:
        self._index = index
        self._node = node
        self._tree = tree
        self._token = token

    def __add__(self, offset: int) -> 'SetBackwardIterator[Domain]':
        return SetBackwardIterator(*self._move_node(offset), self._tree,
                                   self._token)

    def __eq__(self, other: 'SetBackwardIterator[Domain]') -> bool:
        return (self._to_validated_node() is other._to_validated_node()
                if isinstance(other, SetBackwardIterator)
                else NotImplemented)

    def __iadd__(self, offset: int) -> 'SetBackwardIterator[Domain]':
        self._index, self._node = self._move_node(offset)
        return self

    def __isub__(self, offset: int) -> 'SetBackwardIterator[Domain]':
        self._index, self._node = self._move_node(-offset)
        return self

    def __iter__(self) -> 'SetBackwardIterator[Domain]':
        return self

    def __next__(self) -> Domain:
        node = self._to_validated_node()
        try:
            result = node.value
        except AttributeError:
            raise StopIteration from None
        else:
            self._index += 1
            self._node = self._tree.predecessor(self._node)
            return result

    def __sub__(self, offset: int) -> 'SetBackwardIterator[Domain]':
        return SetBackwardIterator(*self._move_node(-offset), self._tree,
                                   self._token)

    def _move_node(self, offset: int) -> Tuple[int, AnyNode]:
        self._validate()
        index = self._index
        size = len(self._tree)
        min_offset, max_offset = -index, size - index
        if offset < min_offset or offset > max_offset:
            raise ValueError('Offset should be '
                             'in range({min_offset}, {max_offset}), '
                             'but found {offset}.'
                             .format(min_offset=min_offset,
                                     max_offset=max_offset + 1,
                                     offset=offset)
                             if size
                             else 'Set is empty.')
        new_index = index + offset
        return new_index, (_index_to_node(size - new_index - 1, self._tree)
                           if new_index < size
                           else red_black.NIL)

    def _to_validated_node(self) -> AnyNode:
        self._validate()
        return self._node

    def _validate(self) -> None:
        if self._token.expired:
            raise ValueError('Iterator is invalidated.')


class SetForwardIterator(Iterator[Domain]):
    __slots__ = '_index', '_node', '_tree', '_token'

    def __init__(self,
                 index: int,
                 node: AnyNode,
                 tree: red_black.Tree[Domain, Domain],
                 token: Token) -> None:
        self._index = index
        self._node = node
        self._tree = tree
        self._token = token

    def __add__(self, offset: int) -> 'SetForwardIterator[Domain]':
        return SetForwardIterator(*self._move_node(offset), self._tree,
                                  self._token)

    def __eq__(self, other: 'SetForwardIterator[Domain]') -> bool:
        return (self._to_validated_node() is other._to_validated_node()
                if isinstance(other, SetForwardIterator)
                else NotImplemented)

    def __iadd__(self, offset: int) -> 'SetForwardIterator[Domain]':
        self._index, self._node = self._move_node(offset)
        return self

    def __isub__(self, offset: int) -> 'SetForwardIterator[Domain]':
        self._index, self._node = self._move_node(-offset)
        return self

    def __iter__(self) -> 'SetForwardIterator[Domain]':
        return self

    def __next__(self) -> Domain:
        node = self._to_validated_node()
        try:
            result = node.value
        except AttributeError:
            raise StopIteration from None
        else:
            self._node = self._tree.successor(self._node)
            return result

    def __sub__(self, offset: int) -> 'SetForwardIterator[Domain]':
        return SetForwardIterator(*self._move_node(-offset), self._tree,
                                  self._token)

    def _move_node(self, offset: int) -> Tuple[int, AnyNode]:
        self._validate()
        index = self._index
        size = len(self._tree)
        min_offset, max_offset = -index, size - index
        if offset < min_offset or offset > max_offset:
            raise ValueError('Offset should be '
                             'in range({min_offset}, {max_offset}), '
                             'but found {offset}.'
                             .format(min_offset=min_offset,
                                     max_offset=max_offset + 1,
                                     offset=offset)
                             if size
                             else 'Set is empty.')
        new_index = index + offset
        return new_index, (_index_to_node(new_index, self._tree)
                           if new_index < size
                           else red_black.NIL)

    def _to_validated_node(self) -> AnyNode:
        self._validate()
        return self._node

    def _validate(self) -> None:
        if self._token.expired:
            raise ValueError('Iterator is invalidated.')


def _node_to_index(node: red_black.Node, tree: red_black.Tree) -> int:
    return next(index
                for index, candidate in enumerate(tree)
                if candidate is node)


def _index_to_node(index: int, tree: red_black.Tree) -> red_black.Node:
    return next(islice(tree, index, None))
