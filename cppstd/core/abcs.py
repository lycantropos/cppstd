from abc import (ABC,
                 abstractmethod)
from typing import Optional

from .hints import Value


class LegacyIterator(ABC):
    @abstractmethod
    def next(self) -> 'LegacyIterator':
        """Returns incremented the iterator."""

    @property
    @abstractmethod
    def value(self) -> Value:
        """Returns underlying value."""


class LegacyInputIterator(LegacyIterator):
    @abstractmethod
    def inc(self) -> Optional['LegacyInputIterator']:
        """Increments the iterator."""


class LegacyOutputIterator(LegacyIterator):
    @abstractmethod
    def inc(self) -> 'LegacyOutputIterator':
        """Increments the iterator and returns previous position."""

    @LegacyIterator.value.setter
    @abstractmethod
    def value(self, value: Value) -> None:
        """Sets underlying value."""


class LegacyForwardIterator(LegacyInputIterator):
    @abstractmethod
    def inc(self) -> 'LegacyForwardIterator':
        """Increments the iterator and returns previous position."""


class LegacyBidirectionalIterator(LegacyForwardIterator):
    @abstractmethod
    def dec(self) -> 'LegacyForwardIterator':
        """Decrements the iterator and returns previous position."""

    @abstractmethod
    def prev(self) -> 'LegacyForwardIterator':
        """Returns decremented the iterator."""


class LegacyRandomAccessIterator(LegacyBidirectionalIterator):
    @abstractmethod
    def __add__(self, offset: int) -> 'LegacyRandomAccessIterator':
        """Returns the iterator moved forward by offset."""

    @abstractmethod
    def __iadd__(self, offset: int) -> 'LegacyRandomAccessIterator':
        """Moves the iterator forward by offset."""

    @abstractmethod
    def __isub__(self, offset: int) -> 'LegacyRandomAccessIterator':
        """Moves the iterator backward by offset."""

    @abstractmethod
    def __le__(self, other: 'LegacyRandomAccessIterator') -> bool:
        """Checks if the iterator is less than or equal to the other."""

    @abstractmethod
    def __lt__(self, other: 'LegacyRandomAccessIterator') -> bool:
        """Checks if the iterator is less than the other."""

    @abstractmethod
    def __radd__(self, offset: int) -> 'LegacyRandomAccessIterator':
        """Returns the iterator moved forward by offset."""

    @abstractmethod
    def __sub__(self, offset: int) -> 'LegacyRandomAccessIterator':
        """Returns the iterator moved backward by offset."""
