from typing import Union

from dendroid.red_black import (NIL,
                                Node,
                                Tree)

from .tokenization import WeakToken

AnyNode = Union[NIL, Node]
Tree = Tree


class BaseTreeIterator:
    __slots__ = '_index', '_node', '_tree', '_token'

    def __init__(self,
                 index: int,
                 node: AnyNode,
                 tree: Tree,
                 token: WeakToken) -> None:
        self._index = index
        self._node = node
        self._tree = tree
        self._token = token

    def __eq__(self, other: 'BaseTreeIterator') -> bool:
        return (self._validate_comparison_with(other)
                or self._to_validated_node() is other._to_validated_node()
                if isinstance(other, type(self))
                else NotImplemented)

    def _to_validated_node(self) -> AnyNode:
        self._validate()
        return self._node

    def _validate(self) -> None:
        if self._token.expired:
            raise RuntimeError('Iterator is invalidated.')

    def _validate_comparison_with(self, other: 'BaseTreeIterator') -> None:
        if self._tree is not other._tree:
            raise RuntimeError('Comparing iterators '
                               'from different collections is undefined.')


class TreeIterator(BaseTreeIterator):
    def dec(self) -> 'TreeIterator':
        node = self._to_validated_node()
        index = self._index
        if not index:
            raise RuntimeError('Post-decrementing of start iterators '
                               'is undefined.')
        self._node = self._tree.predecessor(node)
        self._index -= 1
        return type(self)(index, node, self._tree, self._token)

    def inc(self) -> 'TreeIterator':
        node = self._to_validated_node()
        if node is NIL:
            raise RuntimeError('Post-incrementing of stop iterators '
                               'is undefined.')
        index = self._index
        self._node = self._tree.successor(node)
        self._index += 1
        return type(self)(index, node, self._tree, self._token)


class TreeReverseIterator(BaseTreeIterator):
    def dec(self) -> 'TreeReverseIterator':
        node = self._to_validated_node()
        index = self._index
        if not index:
            raise RuntimeError('Post-decrementing of start iterators '
                               'is undefined.')
        self._node = self._tree.successor(node)
        self._index -= 1
        return type(self)(index, node, self._tree, self._token)

    def inc(self) -> 'TreeReverseIterator':
        node = self._to_validated_node()
        if node is NIL:
            raise RuntimeError('Post-incrementing of stop iterators '
                               'is undefined.')
        index = self._index
        self._node = self._tree.predecessor(node)
        self._index += 1
        return type(self)(index, node, self._tree, self._token)
