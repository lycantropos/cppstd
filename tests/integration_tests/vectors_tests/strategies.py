from hypothesis import strategies

from tests.utils import to_bound_ported_vectors_pair

objects = strategies.integers()
empty_lists = strategies.builds(list)
objects_lists = strategies.lists(objects)
non_empty_objects_lists = strategies.lists(objects,
                                           min_size=1)
vectors_pairs = strategies.builds(to_bound_ported_vectors_pair,
                                  objects_lists)
empty_vectors_pairs = strategies.builds(to_bound_ported_vectors_pair,
                                        empty_lists)
non_empty_vectors_pairs = strategies.builds(to_bound_ported_vectors_pair,
                                            non_empty_objects_lists)
