from typing import Generic

from reprit.base import generate_repr

from .core import red_black
from .core.tokenization import Tokenizer
from .hints import (Item,
                    Key,
                    Value)


class MapForwardIterator(red_black.TreeForwardIterator, Generic[Key, Value]):
    def __next__(self) -> Item:
        return super().__next__().item


class MapKeysBackwardIterator(red_black.TreeBackwardIterator, Generic[Key]):
    def __next__(self) -> Key:
        return super().__next__().key


class MapKeysForwardIterator(red_black.TreeForwardIterator, Generic[Key]):
    def __next__(self) -> Key:
        return super().__next__().key


class Map(Generic[Key, Value]):
    __slots__ = '_raw', '_tokenizer'

    def __init__(self, *items: Item) -> None:
        self._raw = red_black.map_(*items)
        self._tokenizer = Tokenizer()

    __repr__ = generate_repr(__init__)

    def __delitem__(self, key: Key) -> None:
        node = self._raw.tree.find(key)
        if node is red_black.NIL:
            raise ValueError('{!r} is not found.'.format(key))
        else:
            self._tokenizer.reset()
            self._raw.tree.remove(node)

    def __eq__(self, other: 'Map[Key, Value]') -> bool:
        return (self._raw == other._raw
                if isinstance(other, Map)
                else NotImplemented)

    def __len__(self) -> int:
        return len(self._raw)

    def __iter__(self) -> MapKeysForwardIterator[Key]:
        return MapKeysForwardIterator(0, self._raw.tree.min(),
                                      self._raw.tree,
                                      self._tokenizer.create())

    def __reversed__(self) -> MapKeysBackwardIterator[Key]:
        return MapKeysBackwardIterator(0, self._raw.tree.max(),
                                       self._raw.tree,
                                       self._tokenizer.create())

    def items(self) -> MapForwardIterator[Key, Value]:
        return MapForwardIterator(0, self._raw.tree.min(), self._raw.tree,
                                  self._tokenizer.create())

    keys = __iter__
