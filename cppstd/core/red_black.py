from abc import (ABC,
                 abstractmethod)
from itertools import islice
from typing import (Tuple,
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


class BaseTreeIterator(ABC):
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

    def __eq__(self, other: 'BaseTreeIterator[Node]') -> bool:
        return (self._to_validated_node() is other._to_validated_node()
                if isinstance(other, type(self))
                else NotImplemented)

    @abstractmethod
    def _index_to_node(self, index: int, size: int) -> AnyNode:
        """Returns node by given index."""

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
        return new_index, self._index_to_node(new_index, size)

    def _to_validated_node(self) -> AnyNode:
        self._validate()
        return self._node

    def _validate(self) -> None:
        if self._token.expired:
            raise ValueError('Iterator is invalidated.')


class TreeReverseIterator(BaseTreeIterator):
    def _index_to_node(self, index: int, size: int) -> AnyNode:
        return (index_to_node(size - index - 1, self._tree)
                if index < size
                else NIL)


class TreeIterator(BaseTreeIterator):
    def _index_to_node(self, index: int, size: int) -> AnyNode:
        return (index_to_node(index, self._tree)
                if index < size
                else NIL)
