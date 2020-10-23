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
    __slots__ = '_node', '_tree', '_token'

    def __init__(self,
                 node: AnyNode,
                 tree: Tree,
                 token: WeakToken) -> None:
        self._node = node
        self._tree = tree
        self._token = token

    def __eq__(self, other: 'TreeIterator') -> bool:
        return (self._validate_comparison_with(other)
                or self._to_validated_node() is other._to_validated_node()
                if isinstance(other, type(self))
                else NotImplemented)

    def _validate_comparison_with(self, other: 'TreeIterator') -> None:
        if self._tree is not other._tree:
            raise RuntimeError('Comparing iterators '
                               'from different collections is undefined.')

    def _to_validated_node(self) -> AnyNode:
        self._validate()
        return self._node

    def _validate(self) -> None:
        if self._token.expired:
            raise RuntimeError('Iterator is invalidated.')
