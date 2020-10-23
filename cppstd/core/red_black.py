from typing import Union

from dendroid.red_black import (NIL,
                                Node,
                                Tree,
                                map_,
                                set_)

from .tokenization import WeakToken

AnyNode = Union[NIL, Node]
Tree = Tree
map_ = map_
set_ = set_


class TreeIterator:
    __slots__ = '_index', '_node', '_tree', '_token'

    def __init__(self,
                 index: int,
                 node: AnyNode,
                 tree: Tree[Node, Node],
                 token: WeakToken) -> None:
        self._index = index
        self._node = node
        self._tree = tree
        self._token = token

    def __eq__(self, other: 'TreeIterator[Node]') -> bool:
        return (self._to_validated_node() is other._to_validated_node()
                if isinstance(other, type(self))
                else NotImplemented)

    def _to_validated_node(self) -> AnyNode:
        self._validate()
        return self._node

    def _validate(self) -> None:
        if self._token.expired:
            raise RuntimeError('Iterator is invalidated.')
