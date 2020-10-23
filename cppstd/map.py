from typing import Generic

from reprit.base import generate_repr

from .core import red_black
from .core.tokenization import Tokenizer
from .hints import (Item,
                    Key,
                    Value)


class map(Generic[Key, Value]):
    class const_iterator(red_black.TreeIterator, Generic[Item]):
        @property
        def value(self) -> Item:
            node = self._to_validated_node()
            if node is red_black.NIL:
                raise RuntimeError('Getting value of placeholder iterators '
                                   'is undefined.')
            return node.item

    class const_reverse_iterator(red_black.TreeReverseIterator,
                                 Generic[Item]):
        @property
        def value(self) -> Item:
            node = self._to_validated_node()
            if node is red_black.NIL:
                raise RuntimeError('Getting value of placeholder iterators '
                                   'is undefined.')
            return node.item

    iterator = const_iterator
    reverse_iterator = const_reverse_iterator

    __slots__ = '_items', '_tokenizer'

    def __init__(self, *_items: Item) -> None:
        self._items = red_black.map_(*_items)
        self._tokenizer = Tokenizer()

    __repr__ = generate_repr(__init__)

    def __eq__(self, other: 'map[Key, Value]') -> bool:
        return (self._items == other._items
                if isinstance(other, map)
                else NotImplemented)

    def __setitem__(self, key: Key, value: Value) -> None:
        node = self._items.tree.find(key)
        self._tokenizer.reset()
        if node is red_black.NIL:
            self._items[key] = value
        else:
            node.value = value

    def begin(self) -> iterator[Item]:
        return self.iterator(0, self._items.tree.min(), self._items.tree,
                             self._tokenizer.create_weak())

    def cbegin(self) -> const_iterator[Item]:
        return self.iterator(0, self._items.tree.min(), self._items.tree,
                             self._tokenizer.create_weak())

    def cend(self) -> const_iterator[Item]:
        return self.const_iterator(len(self._items), red_black.NIL,
                                   self._items.tree,
                                   self._tokenizer.create_weak())

    def clear(self) -> None:
        self._tokenizer.reset()
        self._items.clear()

    def crbegin(self) -> const_reverse_iterator[Item]:
        return self.const_reverse_iterator(0, self._items.tree.max(),
                                           self._items.tree,
                                           self._tokenizer.create_weak())

    def crend(self) -> const_reverse_iterator[Item]:
        return self.const_reverse_iterator(len(self._items), red_black.NIL,
                                           self._items.tree,
                                           self._tokenizer.create_weak())

    def empty(self) -> bool:
        return not self._items

    def end(self) -> iterator[Item]:
        return self.iterator(len(self._items), red_black.NIL, self._items.tree,
                             self._tokenizer.create_weak())

    def rbegin(self) -> reverse_iterator[Item]:
        return self.reverse_iterator(0, self._items.tree.max(),
                                     self._items.tree,
                                     self._tokenizer.create_weak())

    def rend(self) -> reverse_iterator[Item]:
        return self.reverse_iterator(len(self._items), red_black.NIL,
                                     self._items.tree,
                                     self._tokenizer.create_weak())

    def size(self) -> int:
        return len(self._items)
