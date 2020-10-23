import pickle
from collections import deque
from functools import partial
from itertools import count
from operator import (eq,
                      itemgetter)
from typing import (Any,
                    Callable,
                    Iterable,
                    List,
                    Tuple,
                    TypeVar)

from _cppstd import (map as BoundMap,
                     set as BoundSet,
                     vector as BoundVector)
from hypothesis.strategies import SearchStrategy

from cppstd.hints import (Item,
                          Value)
from cppstd.map import map as PortedMap
from cppstd.set import set as PortedSet
from cppstd.vector import vector as PortedVector

Domain = TypeVar('Domain')
Range = TypeVar('Range')
Strategy = SearchStrategy
BoundMap = BoundMap
BoundSet = BoundSet
BoundVector = BoundVector
PortedMap = PortedMap
PortedSet = PortedSet
PortedVector = PortedVector
BoundPortedMapsPair = Tuple[BoundMap, PortedMap]
BoundPortedSetsPair = Tuple[BoundSet, PortedSet]
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


def pairwise(iterable: Iterable[Domain]) -> Iterable[Tuple[Domain, Domain]]:
    iterator = iter(iterable)
    element = next(iterator, None)
    for next_element in iterator:
        yield element, next_element
        element = next_element


def pickle_round_trip(value: Any) -> Any:
    return pickle.loads(pickle.dumps(value))


item_to_key = itemgetter(0)


def are_bound_ported_maps_equal(bound: BoundMap, ported: PortedMap) -> bool:
    return bound.size() == ported.size() and all(map(eq, bound, ported))


def are_bound_ported_sets_equal(bound: BoundSet, ported: PortedSet) -> bool:
    return bound.size() == ported.size() and all(map(eq, bound, ported))


def are_bound_ported_vectors_equal(bound: BoundVector,
                                   ported: PortedVector) -> bool:
    return bound.size() == ported.size() and all(map(eq, bound, ported))


def to_bound_ported_maps_pair(items: List[Item]) -> BoundPortedMapsPair:
    return BoundMap(*items), PortedMap(*items)


def to_bound_ported_sets_pair(values: List[Value]) -> BoundPortedSetsPair:
    return BoundSet(*values), PortedSet(*values)


def to_bound_ported_vectors_pair(values: List[Value]
                                 ) -> BoundPortedVectorsPair:
    return BoundVector(*values), PortedVector(*values)
