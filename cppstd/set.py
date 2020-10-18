from itertools import islice
from typing import (Generic,
                    Iterator,
                    Union)

from dendroid import red_black
from reprit.base import (generate_repr,
                         seekers)

from .core.tokenization import (Token,
                                Tokenizer)
from .hints import Domain

AnyNode = Union[red_black.NIL, red_black.Node]


class Set(Generic[Domain]):
    __slots__ = '_values', '_tokenizer'

    def __init__(self, *values: Domain) -> None:
        self._values = red_black.set_(*values)
        self._tokenizer = Tokenizer()

    __repr__ = generate_repr(__init__,
                             field_seeker=seekers.complex_)

    def __bool__(self) -> bool:
        return bool(self._values)

    def __eq__(self, other: 'Set[Domain]') -> bool:
        return (self._values == other._values
                if isinstance(other, Set)
                else NotImplemented)

    def __iter__(self) -> 'SetForwardIterator[Domain]':
        return SetForwardIterator(self._values.min(), self._values.tree,
                                  self._tokenizer.create())

    def __len__(self) -> int:
        return len(self._values)


class SetForwardIterator(Iterator[Domain]):
    __slots__ = '_node', '_tree', '_token'

    def __init__(self,
                 node: AnyNode,
                 tree: red_black.Tree[Domain, Domain],
                 token: Token) -> None:
        self._node = node
        self._tree = tree
        self._token = token

    def __add__(self, offset: int) -> 'SetForwardIterator[Domain]':
        return SetForwardIterator(self._move_node(offset), self._tree,
                                  self._token)

    def __iadd__(self, offset: int) -> 'SetForwardIterator[Domain]':
        self._node = self._move_node(offset)
        return self

    def __isub__(self, offset: int) -> 'SetForwardIterator[Domain]':
        self._node = self._move_node(-offset)
        return self

    def __eq__(self, other: 'SetForwardIterator[Domain]') -> bool:
        return (self._to_validated_node() is other._to_validated_node()
                if isinstance(other, SetForwardIterator)
                else NotImplemented)

    def __iter__(self) -> 'SetForwardIterator[Domain]':
        return self

    def __next__(self) -> Domain:
        try:
            result = self._node.value
        except AttributeError:
            raise StopIteration from None
        else:
            self._node = self._tree.successor(self._node)
            return result

    def __sub__(self, offset: int) -> 'SetForwardIterator[Domain]':
        return SetForwardIterator(self._move_node(-offset), self._tree,
                                  self._token)

    def _move_node(self, offset: int) -> AnyNode:
        index = _node_to_index(self._to_validated_node(), self._tree)
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
        return _index_to_node(index + offset, self._tree)

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
    return next(islice(tree, index, None), red_black.NIL)
