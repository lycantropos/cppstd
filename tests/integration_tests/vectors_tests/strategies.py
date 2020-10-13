from hypothesis import strategies

objects = strategies.integers()
objects_lists = strategies.lists(objects)
