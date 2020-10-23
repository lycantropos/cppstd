import pickle
from collections import deque
from functools import partial
from itertools import (count,
                       zip_longest)
from operator import (eq,
                      itemgetter)
from typing import (Any,
                    Callable,
                    Iterable,
                    Iterator,
                    List,
                    Tuple,
                    TypeVar,
                    Union)

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
BoundSetIterator = Union[BoundSet.const_iterator,
                         BoundSet.const_reverse_iterator,
                         BoundSet.iterator, BoundSet.reverse_iterator]
BoundVector = BoundVector
BoundVectorIterator = Union[BoundVector.const_iterator,
                            BoundVector.const_reverse_iterator,
                            BoundVector.iterator, BoundVector.reverse_iterator]
PortedMap = PortedMap
PortedSet = PortedSet
PortedSetIterator = Union[PortedSet.const_iterator,
                          PortedSet.const_reverse_iterator,
                          PortedSet.iterator, PortedSet.reverse_iterator]
PortedVector = PortedVector
PortedVectorIterator = Union[PortedVector.const_iterator,
                             PortedVector.const_reverse_iterator,
                             PortedVector.iterator,
                             PortedVector.reverse_iterator]
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


def are_bound_ported_set_iterators_equal(bound: BoundSetIterator,
                                         bound_stop: BoundSetIterator,
                                         ported: PortedSetIterator,
                                         ported_stop: PortedSetIterator,
                                         ) -> bool:
    return are_bound_ported_iterators_equal(bound, bound_stop, ported,
                                            ported_stop)


def are_bound_ported_vectors_equal(bound: BoundVector,
                                   ported: PortedVector) -> bool:
    return bound.size() == ported.size() and all(map(eq, bound, ported))


def are_bound_ported_vector_iterators_equal(bound: BoundVectorIterator,
                                            bound_stop: BoundVectorIterator,
                                            ported: PortedVectorIterator,
                                            ported_stop: PortedVectorIterator,
                                            ) -> bool:
    return are_bound_ported_iterators_equal(bound, bound_stop, ported,
                                            ported_stop)


def are_bound_ported_iterators_equal(bound: Domain,
                                     bound_stop: Domain,
                                     ported: Range,
                                     ported_stop: Range) -> bool:
    return all(bound_iterator is not None
               and ported_iterator is not None
               and bound_iterator.value == ported_iterator.value
               for bound_iterator, ported_iterator
               in zip_longest(iterate_cpp_iterator(bound, bound_stop),
                              iterate_cpp_iterator(ported, ported_stop),
                              fillvalue=None))


def iterate_cpp_iterator(start: Domain, stop: Domain) -> Iterator[Domain]:
    while start != stop:
        yield start.inc()


def to_bound_ported_maps_pair(items: List[Item]) -> BoundPortedMapsPair:
    return BoundMap(*items), PortedMap(*items)


def to_bound_ported_sets_pair(values: List[Value]) -> BoundPortedSetsPair:
    return BoundSet(*values), PortedSet(*values)


def to_bound_ported_vectors_pair(values: List[Value]
                                 ) -> BoundPortedVectorsPair:
    return BoundVector(*values), PortedVector(*values)
