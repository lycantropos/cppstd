from collections import deque
from functools import partial
from itertools import count
from operator import eq
from typing import (Any,
                    Callable,
                    Iterable,
                    Tuple,
                    TypeVar)

from _cppstd import Vector as BoundVector
from hypothesis.strategies import SearchStrategy

from cppstd.vector import Vector as PortedVector

Domain = TypeVar('Domain')
Range = TypeVar('Range')
Strategy = SearchStrategy
BoundVector = BoundVector
PortedVector = PortedVector
BoundPortedVectorsPair = Tuple[BoundVector, PortedVector]


def equivalence(left: bool, right: bool) -> bool:
    return left is right


def implication(left: bool, right: bool) -> bool:
    return not left or right


def identity(value: Domain) -> Domain:
    return value


def pack(function: Callable[..., Range]
         ) -> Callable[[Iterable[Domain]], Range]:
    return partial(apply, function)


def apply(function: Callable[..., Range],
          args: Iterable[Domain]) -> Range:
    return function(*args)


def capacity(iterable: Iterable[Any]) -> int:
    counter = count()
    # order matters: if `counter` goes first,
    # then it will be incremented even for empty `iterable`
    deque(zip(iterable, counter),
          maxlen=0)
    return next(counter)


def are_bound_ported_vectors_equal(bound: BoundVector,
                                   ported: PortedVector) -> bool:
    return len(bound) == len(ported) and all(map(eq, bound, ported))
