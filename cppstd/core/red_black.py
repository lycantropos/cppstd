from itertools import islice
from typing import Union

from dendroid.red_black import (NIL,
                                Node,
                                Tree,
                                set_)

AnyNode = Union[NIL, Node]
Tree = Tree
set_ = set_


def index_to_node(index: int, tree: Tree) -> Node:
    return next(islice(tree, index, None))
