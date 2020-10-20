from itertools import islice
from typing import (Iterator,
                    Tuple,
                    Union)

from dendroid.red_black import (NIL,
                                Node,
                                Tree,
                                map_,
                                set_)

from .tokenization import Token

AnyNode = Union[NIL, Node]
Tree = Tree
map_ = map_
set_ = set_


def index_to_node(index: int, tree: Tree) -> Node:
    return next(islice(tree, index, None))


class TreeBackwardIterator(Iterator[Node]):
    __slots__ = '_index', '_node', '_tree', '_token'

    def __init__(self,
                 index: int,
                 node: AnyNode,
                 tree: Tree[Node, Node],
                 token: Token) -> None:
        self._index = index
        self._node = node
        self._tree = tree
        self._token = token

    def __add__(self, offset: int) -> 'TreeBackwardIterator[Node]':
        return TreeBackwardIterator(*self._move_node(offset), self._tree,
                                    self._token)

    def __eq__(self, other: 'TreeBackwardIterator[Node]') -> bool:
        return (self._to_validated_node() is other._to_validated_node()
                if isinstance(other, TreeBackwardIterator)
                else NotImplemented)

    def __iadd__(self, offset: int) -> 'TreeBackwardIterator[Node]':
        self._index, self._node = self._move_node(offset)
        return self

    def __isub__(self, offset: int) -> 'TreeBackwardIterator[Node]':
        self._index, self._node = self._move_node(-offset)
        return self

    def __iter__(self) -> 'TreeBackwardIterator[Node]':
        return self

    def __next__(self) -> Node:
        result = self._to_validated_node()
        if result is NIL:
            raise StopIteration from None
        self._index += 1
        self._node = self._tree.predecessor(self._node)
        return result

    def __sub__(self, offset: int) -> 'TreeBackwardIterator[Node]':
        return TreeBackwardIterator(*self._move_node(-offset), self._tree,
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
                             else 'Container is empty.')
        new_index = index + offset
        return new_index, (index_to_node(size - new_index - 1, self._tree)
                           if new_index < size
                           else NIL)

    def _to_validated_node(self) -> AnyNode:
        self._validate()
        return self._node

    def _validate(self) -> None:
        if self._token.expired:
            raise ValueError('Iterator is invalidated.')


class TreeForwardIterator(Iterator[Node]):
    __slots__ = '_index', '_node', '_tree', '_token'

    def __init__(self,
                 index: int,
                 node: AnyNode,
                 tree: Tree[Node, Node],
                 token: Token) -> None:
        self._index = index
        self._node = node
        self._tree = tree
        self._token = token

    def __add__(self, offset: int) -> 'TreeForwardIterator[Node]':
        return TreeForwardIterator(*self._move_node(offset), self._tree,
                                   self._token)

    def __eq__(self, other: 'TreeForwardIterator[Node]') -> bool:
        return (self._to_validated_node() is other._to_validated_node()
                if isinstance(other, TreeForwardIterator)
                else NotImplemented)

    def __iadd__(self, offset: int) -> 'TreeForwardIterator[Node]':
        self._index, self._node = self._move_node(offset)
        return self

    def __isub__(self, offset: int) -> 'TreeForwardIterator[Node]':
        self._index, self._node = self._move_node(-offset)
        return self

    def __iter__(self) -> 'TreeForwardIterator[Node]':
        return self

    def __next__(self) -> Node:
        result = self._to_validated_node()
        if result is NIL:
            raise StopIteration from None
        self._index += 1
        self._node = self._tree.successor(self._node)
        return result

    def __sub__(self, offset: int) -> 'TreeForwardIterator[Node]':
        return TreeForwardIterator(*self._move_node(-offset), self._tree,
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
                             else 'Container is empty.')
        new_index = index + offset
        return new_index, (index_to_node(new_index, self._tree)
                           if new_index < size
                           else NIL)

    def _to_validated_node(self) -> AnyNode:
        self._validate()
        return self._node

    def _validate(self) -> None:
        if self._token.expired:
            raise ValueError('Iterator is invalidated.')
