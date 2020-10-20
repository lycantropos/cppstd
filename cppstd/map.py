from typing import Generic

from reprit.base import generate_repr

from .core import red_black
from .core.tokenization import Tokenizer
from .hints import (Item,
                    Key,
                    Value)


class Map(Generic[Key, Value]):
    __slots__ = '_items', '_tokenizer'

    def __init__(self, *items: Item) -> None:
        self._items = red_black.map_(*items)
        self._tokenizer = Tokenizer()

    __repr__ = generate_repr(__init__)

    def items(self) -> 'MapForwardIterator[Key, Value]':
        return MapForwardIterator(0, self._items.tree.min(), self._items.tree,
                                  self._tokenizer.create())


class MapForwardIterator(red_black.TreeForwardIterator, Generic[Key, Value]):
    def __next__(self) -> Item:
        return super().__next__().item
