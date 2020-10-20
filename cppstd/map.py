from typing import Generic

from dendroid import red_black
from reprit.base import generate_repr

from .hints import (Item,
                    Key,
                    Value)


class Map(Generic[Key, Value]):
    def __init__(self, *items: Item) -> None:
        self._items = red_black.map_(*items)

    __repr__ = generate_repr(__init__)
