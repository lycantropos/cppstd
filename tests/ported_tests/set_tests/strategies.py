from hypothesis import strategies

from tests.utils import (PortedSet,
                         pack)

objects = strategies.integers()
objects_lists = strategies.lists(objects)
sets = objects_lists.map(pack(PortedSet))
