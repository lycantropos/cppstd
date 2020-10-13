from hypothesis import strategies

from tests.utils import to_bound_ported_vectors_pair

objects = strategies.integers()
objects_lists = strategies.lists(objects)
vectors_pairs = strategies.builds(to_bound_ported_vectors_pair,
                                  objects_lists)
