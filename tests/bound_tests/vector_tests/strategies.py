from hypothesis import strategies

from tests.utils import (BoundVector,
                         pack)

objects = strategies.integers()
objects_lists = strategies.lists(objects)
vectors = objects_lists.map(pack(BoundVector))
