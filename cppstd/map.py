import builtins
from typing import (Generic,
                    Iterator)

from .core import red_black
from .core.tokenization import (SharedToken,
                                Tokenizer)
from .core.utils import identity
from .hints import (Item,
                    Key,
                    Value)


class map_iterator(Iterator[Value]):
    __slots__ = '_node', '_tree', '_token'

    def __init__(self,
                 node: red_black.AnyNode,
                 tree: red_black.Tree,
                 token: SharedToken) -> None:
        self._node = node
        self._tree = tree
        self._token = token

    __iter__ = identity

    def __next__(self) -> Item:
        if self._token.expired:
            raise RuntimeError('Iterator is invalidated.')
        if self._node is red_black.NIL:
            raise StopIteration
        item = self._node.item
        self._node = self._tree.successor(self._node)
        return item


class map(Generic[Key, Value]):
    class const_iterator(red_black.TreeIterator, Generic[Key, Value]):
        @property
        def value(self) -> Item:
            node = self._to_validated_node()
            if node is red_black.NIL:
                raise RuntimeError('Getting value of stop iterators '
                                   'is undefined.')
            return node.item

    class const_reverse_iterator(red_black.TreeReverseIterator,
                                 Generic[Key, Value]):
        @property
        def value(self) -> Item:
            node = self._to_validated_node()
            if node is red_black.NIL:
                raise RuntimeError('Getting value of stop iterators '
                                   'is undefined.')
            return node.item

    iterator = const_iterator
    reverse_iterator = const_reverse_iterator

    __slots__ = '_tree', '_tokenizer'

    def __init__(self, *_items: Item) -> None:
        self._tree = red_black.Tree.from_components(*(zip(*_items)
                                                      if _items
                                                      else ((), ())))
        self._tokenizer = Tokenizer()

    def __repr__(self) -> str:
        return (type(self).__qualname__ + '('
                + ', '.join(builtins.map(repr, self)) + ')')

    def __eq__(self, other: 'map[Key, Value]') -> bool:
        return (self._tree.values == other._tree.values
                if isinstance(other, map)
                else NotImplemented)

    def __iter__(self) -> map_iterator[Value]:
        return map_iterator(self._tree.min(), self._tree,
                            self._tokenizer.create_shared())

    def __setitem__(self, key: Key, value: Value) -> None:
        node = self._tree.find(key)
        self._tokenizer.reset()
        if node is red_black.NIL:
            self._tree.insert(key, value)
        else:
            node.value = value

    def begin(self) -> iterator[Key, Value]:
        return self.iterator(0, self._tree.min(), self._tree,
                             self._tokenizer.create_weak())

    def cbegin(self) -> const_iterator[Key, Value]:
        return self.iterator(0, self._tree.min(), self._tree,
                             self._tokenizer.create_weak())

    def cend(self) -> const_iterator[Key, Value]:
        return self.const_iterator(len(self._tree), red_black.NIL, self._tree,
                                   self._tokenizer.create_weak())

    def clear(self) -> None:
        self._tokenizer.reset()
        self._tree.clear()

    def crbegin(self) -> const_reverse_iterator[Key, Value]:
        return self.const_reverse_iterator(0, self._tree.max(), self._tree,
                                           self._tokenizer.create_weak())

    def crend(self) -> const_reverse_iterator[Key, Value]:
        return self.const_reverse_iterator(len(self._tree), red_black.NIL,
                                           self._tree,
                                           self._tokenizer.create_weak())

    def empty(self) -> bool:
        return not self._tree

    def end(self) -> iterator[Key, Value]:
        return self.iterator(len(self._tree), red_black.NIL, self._tree,
                             self._tokenizer.create_weak())

    def rbegin(self) -> reverse_iterator[Key, Value]:
        return self.reverse_iterator(0, self._tree.max(),
                                     self._tree, self._tokenizer.create_weak())

    def rend(self) -> reverse_iterator[Key, Value]:
        return self.reverse_iterator(len(self._tree), red_black.NIL,
                                     self._tree, self._tokenizer.create_weak())

    def size(self) -> int:
        return len(self._tree)
