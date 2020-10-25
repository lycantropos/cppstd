from reprlib import recursive_repr
from typing import (Any,
                    Callable,
                    Iterable,
                    Iterator,
                    List,
                    Optional,
                    Tuple,
                    Union)

from reprit.base import generate_repr

from .hints import (Item,
                    Key,
                    Value)
from .tokenization import WeakToken
from .utils import (dereference_maybe,
                    floor_log2 as to_balanced_tree_height,
                    maybe_weakref,
                    to_unique_sorted_items,
                    to_unique_sorted_values)

NIL = None


class Node:
    __slots__ = ('_key', 'value', 'is_black', '_parent', '_left', '_right',
                 '__weakref__')

    def __init__(self,
                 key: Key,
                 value: Value,
                 is_black: bool,
                 left: Union[NIL, 'Node'] = NIL,
                 right: Union[NIL, 'Node'] = NIL,
                 parent: Union[NIL, 'Node'] = NIL) -> None:
        self._key, self.value, self.is_black = key, value, is_black
        self.left, self.right, self._parent = left, right, parent

    __repr__ = recursive_repr()(generate_repr(__init__))

    State = Tuple[Any, ...]

    def __getstate__(self) -> State:
        return (self._key, self.value, self.is_black,
                self.parent, self._left, self._right)

    def __setstate__(self, state: State) -> None:
        (self._key, self.value, self.is_black,
         self.parent, self._left, self._right) = state

    @classmethod
    def from_simple(cls, key: Key, *args: Any) -> 'Node':
        return cls(key, None, *args)

    @property
    def item(self) -> Item:
        return self._key, self.value

    @property
    def key(self) -> Key:
        return self._key

    @property
    def left(self) -> Union[NIL, 'Node']:
        return self._left

    @left.setter
    def left(self, node: Union[NIL, 'Node']) -> None:
        self._left = node
        _set_parent(node, self)

    @property
    def parent(self) -> Optional['Node']:
        return dereference_maybe(self._parent)

    @parent.setter
    def parent(self, node: Optional['Node']) -> None:
        self._parent = maybe_weakref(node)

    @property
    def right(self) -> Union[NIL, 'Node']:
        return self._right

    @right.setter
    def right(self, node: Union[NIL, 'Node']) -> None:
        self._right = node
        _set_parent(node, self)


AnyNode = Union[NIL, Node]


class Tree:
    __slots__ = 'root', 'size', 'min', 'max'

    def __init__(self,
                 root: AnyNode,
                 size: int,
                 min_node: AnyNode,
                 max_node: AnyNode) -> None:
        self.root = root
        self.size = size
        self.min = min_node
        self.max = max_node

    __repr__ = generate_repr(__init__)

    def __iter__(self) -> Iterator[Node]:
        node = self.root
        queue = []
        while True:
            while node is not NIL:
                queue.append(node)
                node = node.left
            if not queue:
                return
            node = queue.pop()
            yield node
            node = node.right

    def __len__(self) -> int:
        return self.size

    @staticmethod
    def predecessor(node: Node) -> Node:
        if node.left is NIL:
            result = node.parent
            while result is not NIL and node is result.left:
                node, result = result, result.parent
        else:
            result = node.left
            while result.right is not NIL:
                result = result.right
        return result

    @staticmethod
    def successor(node: Node) -> Node:
        if node.right is NIL:
            result = node.parent
            while result is not NIL and node is result.right:
                node, result = result, result.parent
        else:
            result = node.right
            while result.left is not NIL:
                result = result.left
        return result

    @classmethod
    def from_components(cls,
                        keys: Iterable[Key],
                        values: Optional[Iterable[Value]] = None
                        ) -> 'Tree[Key, Value]':
        keys = list(keys)
        if not keys:
            root = min_node = max_node = NIL
            size = 0
        elif values is None:
            keys = to_unique_sorted_values(keys)
            size = len(keys)
            min_node = max_node = NIL

            def to_node(start_index: int,
                        end_index: int,
                        depth: int,
                        height: int = to_balanced_tree_height(size),
                        max_index: int = size - 1,
                        constructor: Callable[..., Node] = Node.from_simple
                        ) -> Node:
                middle_index = (start_index + end_index) // 2
                result = constructor(
                        keys[middle_index], depth != height,
                        (to_node(start_index, middle_index, depth + 1)
                         if middle_index > start_index
                         else NIL),
                        (to_node(middle_index + 1, end_index, depth + 1)
                         if middle_index < end_index - 1
                         else NIL))
                if not middle_index:
                    nonlocal min_node
                    min_node = result
                if middle_index == max_index:
                    nonlocal max_node
                    max_node = result
                return result

            root = to_node(0, size, 0)
            root.is_black = True
        else:
            items = to_unique_sorted_items(keys, tuple(values))
            size = len(items)
            min_node = max_node = NIL

            def to_node(start_index: int,
                        end_index: int,
                        depth: int,
                        height: int = to_balanced_tree_height(size),
                        max_index: int = size - 1,
                        constructor: Callable[..., Node] = Node) -> Node:
                middle_index = (start_index + end_index) // 2
                result = constructor(
                        *items[middle_index], depth != height,
                        (to_node(start_index, middle_index, depth + 1)
                         if middle_index > start_index
                         else NIL),
                        (to_node(middle_index + 1, end_index, depth + 1)
                         if middle_index < end_index - 1
                         else NIL))
                if not middle_index:
                    nonlocal min_node
                    min_node = result
                if middle_index == max_index:
                    nonlocal max_node
                    max_node = result
                return result

            root = to_node(0, size, 0)
            root.is_black = True
        return cls(root, size, min_node, max_node)

    @property
    def items(self) -> List[Value]:
        return [node.item for node in self]

    @property
    def values(self) -> List[Value]:
        return [node.value for node in self]

    def clear(self) -> None:
        self.root = self.min = self.max = NIL
        self.size = 0

    def find(self, key: Key) -> AnyNode:
        node = self.root
        while node is not NIL:
            if key < node.key:
                node = node.left
            elif node.key < key:
                node = node.right
            else:
                break
        return node

    def insert(self, key: Key, value: Value) -> Tuple[Node, bool]:
        parent = self.root
        if parent is NIL:
            node = self.root = self.min = self.max = Node(key, value, True)
            self.size = 1
            return node, True
        while True:
            if key < parent.key:
                if parent.left is NIL:
                    node = Node(key, value, False)
                    parent.left = node
                    break
                else:
                    parent = parent.left
            elif parent.key < key:
                if parent.right is NIL:
                    node = Node(key, value, False)
                    parent.right = node
                    break
                else:
                    parent = parent.right
            else:
                return parent, False
        self._restore(node)
        self.size += 1
        if key < self.min.key:
            self.min = node
        elif self.max.key < key:
            self.max = node
        return node, True

    def remove(self, node: Node) -> None:
        if node is self.min:
            self.min = self.successor(node)
        if node is self.max:
            self.max = self.predecessor(node)
        successor, is_node_black = node, node.is_black
        if successor.left is NIL:
            (successor_child, successor_child_parent,
             is_successor_child_left) = (successor.right, successor.parent,
                                         _is_left_child(successor))
            self._transplant(successor, successor_child)
        elif successor.right is NIL:
            (successor_child, successor_child_parent,
             is_successor_child_left) = (successor.left, successor.parent,
                                         _is_left_child(successor))
            self._transplant(successor, successor_child)
        else:
            successor = node.right
            while successor.left is not NIL:
                successor = successor.left
            is_node_black = successor.is_black
            successor_child, is_successor_child_left = successor.right, False
            if successor.parent is node:
                successor_child_parent = successor
            else:
                is_successor_child_left = _is_left_child(successor)
                successor_child_parent = successor.parent
                self._transplant(successor, successor.right)
                successor.right = node.right
            self._transplant(node, successor)
            successor.left, successor.left.parent = node.left, successor
            successor.is_black = node.is_black
        if is_node_black:
            self._remove_node_fixup(successor_child, successor_child_parent,
                                    is_successor_child_left)
        self.size -= 1

    def _restore(self, node: Node) -> None:
        while not _is_node_black(node.parent):
            parent = node.parent
            grandparent = parent.parent
            if parent is grandparent.left:
                uncle = grandparent.right
                if _is_node_black(uncle):
                    if node is parent.right:
                        self._rotate_left(parent)
                        node, parent = parent, node
                    parent.is_black, grandparent.is_black = True, False
                    self._rotate_right(grandparent)
                else:
                    parent.is_black = uncle.is_black = True
                    grandparent.is_black = False
                    node = grandparent
            else:
                uncle = grandparent.left
                if _is_node_black(uncle):
                    if node is parent.left:
                        self._rotate_right(parent)
                        node, parent = parent, node
                    parent.is_black, grandparent.is_black = True, False
                    self._rotate_left(grandparent)
                else:
                    parent.is_black = uncle.is_black = True
                    grandparent.is_black = False
                    node = grandparent
        self.root.is_black = True

    def _remove_node_fixup(self, node: Union[NIL, Node], parent: Node,
                           is_left_child: bool) -> None:
        while node is not self.root and _is_node_black(node):
            if is_left_child:
                sibling = parent.right
                if not _is_node_black(sibling):
                    sibling.is_black, parent.is_black = True, False
                    self._rotate_left(parent)
                    sibling = parent.right
                if (_is_node_black(sibling.left)
                        and _is_node_black(sibling.right)):
                    sibling.is_black = False
                    node, parent = parent, parent.parent
                    is_left_child = _is_left_child(node)
                else:
                    if _is_node_black(sibling.right):
                        sibling.left.is_black, sibling.is_black = True, False
                        self._rotate_right(sibling)
                        sibling = parent.right
                    sibling.is_black, parent.is_black = parent.is_black, True
                    _set_black(sibling.right)
                    self._rotate_left(parent)
                    node = self.root
            else:
                sibling = parent.left
                if not _is_node_black(sibling):
                    sibling.is_black, parent.is_black = True, False
                    self._rotate_right(parent)
                    sibling = parent.left
                if (_is_node_black(sibling.left)
                        and _is_node_black(sibling.right)):
                    sibling.is_black = False
                    node, parent = parent, parent.parent
                    is_left_child = _is_left_child(node)
                else:
                    if _is_node_black(sibling.left):
                        sibling.right.is_black, sibling.is_black = True, False
                        self._rotate_left(sibling)
                        sibling = parent.left
                    sibling.is_black, parent.is_black = parent.is_black, True
                    _set_black(sibling.left)
                    self._rotate_right(parent)
                    node = self.root
        _set_black(node)

    def _rotate_left(self, node: Node) -> None:
        replacement = node.right
        self._transplant(node, replacement)
        node.right, replacement.left = replacement.left, node

    def _rotate_right(self, node: Node) -> None:
        replacement = node.left
        self._transplant(node, replacement)
        node.left, replacement.right = replacement.right, node

    def _transplant(self, origin: Node, replacement: Union[NIL, Node]) -> None:
        parent = origin.parent
        if parent is None:
            self.root = replacement
            _set_parent(replacement, None)
        elif origin is parent.left:
            parent.left = replacement
        else:
            parent.right = replacement


class BaseTreeIterator:
    __slots__ = '_node', '_tree', '_token'

    def __init__(self,
                 node: AnyNode,
                 tree: Tree,
                 token: WeakToken) -> None:
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
        if node is self._tree.min:
            raise RuntimeError('Post-decrementing of start iterators '
                               'is undefined.')
        self._node = (self._tree.max
                      if node is NIL
                      else self._tree.predecessor(node))
        return type(self)(node, self._tree, self._token)

    def inc(self) -> 'TreeIterator':
        node = self._to_validated_node()
        if node is NIL:
            raise RuntimeError('Post-incrementing of stop iterators '
                               'is undefined.')
        self._node = self._tree.successor(node)
        return type(self)(node, self._tree, self._token)


class TreeReverseIterator(BaseTreeIterator):
    def dec(self) -> 'TreeReverseIterator':
        node = self._to_validated_node()
        if node is self._tree.max:
            raise RuntimeError('Post-decrementing of start iterators '
                               'is undefined.')
        self._node = (self._tree.min
                      if node is NIL
                      else self._tree.successor(node))
        return type(self)(node, self._tree, self._token)

    def inc(self) -> 'TreeReverseIterator':
        node = self._to_validated_node()
        if node is NIL:
            raise RuntimeError('Post-incrementing of stop iterators '
                               'is undefined.')
        self._node = self._tree.predecessor(node)
        return type(self)(node, self._tree, self._token)


def _is_left_child(node: Node) -> bool:
    parent = node.parent
    return parent is not None and parent.left is node


def _is_node_black(node: Union[NIL, Node]) -> bool:
    return node is NIL or node.is_black


def _set_black(maybe_node: Optional[Node]) -> None:
    if maybe_node is not None:
        maybe_node.is_black = True


def _set_parent(node: Union[NIL, Node], parent: Optional[Node]) -> None:
    if node is not NIL:
        node.parent = parent
