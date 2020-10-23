from typing import (Generic,
                    Iterator)

from reprit.base import (generate_repr,
                         seekers)

from .core import red_black
from .core.tokenization import (SharedToken,
                                Tokenizer)
from .core.utils import (identity,
                         lexicographically_lower_than,
                         lexicographically_lower_than_or_equal)
from .hints import Value


class set_iterator(Iterator[Value]):
    __slots__ = '_node', '_tree', '_token'

    def __init__(self,
                 node: red_black.AnyNode,
                 tree: red_black.Tree,
                 token: SharedToken) -> None:
        self._node = node
        self._tree = tree
        self._token = token

    __iter__ = identity

    def __next__(self) -> Value:
        if self._token.expired:
            raise RuntimeError('Iterator is invalidated.')
        if self._node is red_black.NIL:
            raise StopIteration
        value = self._node.value
        self._node = self._tree.successor(self._node)
        return value


class set(Generic[Value]):
    class const_iterator(red_black.TreeIterator, Generic[Value]):
        @property
        def value(self) -> Value:
            node = self._to_validated_node()
            if node is red_black.NIL:
                raise RuntimeError('Getting value of stop iterators '
                                   'is undefined.')
            return node.value

    class const_reverse_iterator(red_black.TreeIterator, Generic[Value]):
        @property
        def value(self) -> Value:
            node = self._to_validated_node()
            if node is red_black.NIL:
                raise RuntimeError('Getting value of stop iterators '
                                   'is undefined.')
            return node.value

    iterator = const_iterator
    reverse_iterator = const_reverse_iterator

    __slots__ = '_values', '_tokenizer'

    def __init__(self, *_values: Value) -> None:
        self._values = red_black.set_(*_values)
        self._tokenizer = Tokenizer()

    __repr__ = generate_repr(__init__,
                             field_seeker=seekers.complex_)

    def __eq__(self, other: 'set[Value]') -> bool:
        return (self._values == other._values
                if isinstance(other, set)
                else NotImplemented)

    def __iter__(self) -> set_iterator[Value]:
        return set_iterator(self._values.tree.min(), self._values.tree,
                            self._tokenizer.create_shared())

    def __le__(self, other: 'set[Value]') -> bool:
        return (lexicographically_lower_than_or_equal(self._values,
                                                      other._values)
                if isinstance(other, set)
                else NotImplemented)

    def __lt__(self, other: 'set[Value]') -> bool:
        return (lexicographically_lower_than(self._values, other._values)
                if isinstance(other, set)
                else NotImplemented)

    def begin(self) -> iterator[Value]:
        return self.iterator(0, self._values.tree.min(), self._values.tree,
                             self._tokenizer.create_weak())

    def cbegin(self) -> const_iterator[Value]:
        return self.const_iterator(0, self._values.tree.min(),
                                   self._values.tree,
                                   self._tokenizer.create_weak())

    def cend(self) -> const_iterator[Value]:
        return self.const_iterator(len(self._values), red_black.NIL,
                                   self._values.tree,
                                   self._tokenizer.create_weak())

    def clear(self) -> None:
        self._tokenizer.reset()
        self._values.clear()

    def crbegin(self) -> const_reverse_iterator[Value]:
        return self.const_reverse_iterator(0, self._values.tree.max(),
                                           self._values.tree,
                                           self._tokenizer.create_weak())

    def crend(self) -> const_reverse_iterator[Value]:
        return self.const_reverse_iterator(len(self._values), red_black.NIL,
                                           self._values.tree,
                                           self._tokenizer.create_weak())

    def empty(self) -> bool:
        return not self._values

    def end(self) -> iterator[Value]:
        return self.iterator(len(self._values), red_black.NIL,
                             self._values.tree,
                             self._tokenizer.create_weak())

    def rbegin(self) -> reverse_iterator[Value]:
        return self.reverse_iterator(0, self._values.tree.max(),
                                     self._values.tree,
                                     self._tokenizer.create_weak())

    def rend(self) -> reverse_iterator[Value]:
        return self.reverse_iterator(len(self._values), red_black.NIL,
                                     self._values.tree,
                                     self._tokenizer.create_weak())

    def size(self) -> int:
        return len(self._values)
