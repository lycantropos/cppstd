import builtins
from typing import (Generic,
                    Iterator)

from .core import red_black
from .core.tokenization import (SharedToken,
                                Tokenizer)
from .core.utils import identity
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
            return node.key

    class const_reverse_iterator(red_black.TreeReverseIterator,
                                 Generic[Value]):
        @property
        def value(self) -> Value:
            node = self._to_validated_node()
            if node is red_black.NIL:
                raise RuntimeError('Getting value of stop iterators '
                                   'is undefined.')
            return node.key

    iterator = const_iterator
    reverse_iterator = const_reverse_iterator

    __slots__ = '_tree', '_tokenizer'

    def __init__(self, *_values: Value) -> None:
        self._tree = red_black.Tree.from_components(_values)
        self._tokenizer = Tokenizer()

    def __eq__(self, other: 'set[Value]') -> bool:
        return (self._tree.values == other._tree.values
                if isinstance(other, set)
                else NotImplemented)

    def __iter__(self) -> set_iterator[Value]:
        return set_iterator(self._tree.min(), self._tree,
                            self._tokenizer.create_shared())

    def __le__(self, other: 'set[Value]') -> bool:
        return (self._tree.values <= other._tree.values
                if isinstance(other, set)
                else NotImplemented)

    def __lt__(self, other: 'set[Value]') -> bool:
        return (self._tree.values < other._tree.values
                if isinstance(other, set)
                else NotImplemented)

    def __repr__(self) -> str:
        return (type(self).__qualname__ + '('
                + ', '.join(builtins.map(repr, self)) + ')')

    def begin(self) -> iterator[Value]:
        return self.iterator(0, self._tree.min(), self._tree,
                             self._tokenizer.create_weak())

    def cbegin(self) -> const_iterator[Value]:
        return self.const_iterator(0, self._tree.min(), self._tree,
                                   self._tokenizer.create_weak())

    def cend(self) -> const_iterator[Value]:
        return self.const_iterator(len(self._tree), red_black.NIL, self._tree,
                                   self._tokenizer.create_weak())

    def clear(self) -> None:
        self._tokenizer.reset()
        self._tree.clear()

    def crbegin(self) -> const_reverse_iterator[Value]:
        return self.const_reverse_iterator(0, self._tree.max(), self._tree,
                                           self._tokenizer.create_weak())

    def crend(self) -> const_reverse_iterator[Value]:
        return self.const_reverse_iterator(len(self._tree), red_black.NIL,
                                           self._tree,
                                           self._tokenizer.create_weak())

    def empty(self) -> bool:
        return not self._tree

    def end(self) -> iterator[Value]:
        return self.iterator(len(self._tree), red_black.NIL, self._tree,
                             self._tokenizer.create_weak())

    def rbegin(self) -> reverse_iterator[Value]:
        return self.reverse_iterator(0, self._tree.max(), self._tree,
                                     self._tokenizer.create_weak())

    def rend(self) -> reverse_iterator[Value]:
        return self.reverse_iterator(len(self._tree), red_black.NIL,
                                     self._tree, self._tokenizer.create_weak())

    def size(self) -> int:
        return len(self._tree)
