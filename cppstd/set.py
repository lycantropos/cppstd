from itertools import islice
from typing import (Generic,
                    Iterator,
                    Union)

from dendroid import red_black
from dendroid.hints import Set as RawSet
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

    def __iter__(self) -> 'SetForwardIterator[Domain]':
        return SetForwardIterator(self._values.tree.min(), self._values)

    def __len__(self) -> int:
        return len(self._values)


class SetForwardIterator(Iterator[Domain]):
    __slots__ = '_node', '_values'

    def __init__(self,
                 node: Union[red_black.NIL, red_black.Node],
                 values: RawSet[Domain]) -> None:
        self._node = node
        self._values = values

    def __add__(self, offset: int) -> 'SetForwardIterator[Domain]':
        return SetForwardIterator(self._move_node(offset), self._values)

    def __iadd__(self, offset: int) -> 'SetForwardIterator[Domain]':
        self._node = self._move_node(offset)
        return self

    def __isub__(self, offset: int) -> 'SetForwardIterator[Domain]':
        self._index = self._move_node(-offset)
        return self

    def __iter__(self) -> 'SetForwardIterator[Domain]':
        return self

    def __le__(self, other: 'SetForwardIterator[Domain]') -> bool:
        return (self._values is other._values and self._index <= other._index
                if isinstance(other, SetForwardIterator)
                else NotImplemented)

    def __lt__(self, other: 'SetForwardIterator[Domain]') -> bool:
        return (self._values is other._values and self._index < other._index
                if isinstance(other, SetForwardIterator)
                else NotImplemented)

    def __next__(self) -> Domain:
        try:
            result = self._node.value
        except AttributeError:
            raise StopIteration from None
        else:
            self._node = self._values.tree.successor(self._node)
            return result

    def __sub__(self, offset: int) -> 'SetForwardIterator[Domain]':
        return SetForwardIterator(self._move_node(-offset), self._values)

    def _move_node(self, offset: int) -> Union[red_black.NIL, red_black.Node]:
        size = len(self._values)
        index = _node_to_index(self._node, self._values.tree)
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
        return _index_to_node(index + offset, self._values.tree)


def _node_to_index(node: red_black.Node, tree: red_black.Tree) -> int:
    return next(index
                for index, candidate in enumerate(tree)
                if candidate is node)


def _index_to_node(index: int, tree: red_black.Tree) -> red_black.Node:
    return next(islice(tree, index, None), red_black.NIL)
