import weakref
from typing import Callable

try:
    from typing import Protocol
except ImportError:
    from typing_extensions import Protocol


class WeakReferencable(Protocol):
    __weakref__ = ...


class Token:
    __slots__ = '_value',

    def __init__(self, value: WeakReferencable) -> None:
        self._value = weakref.ref(value)

    @property
    def expired(self) -> bool:
        return self._value() is None


class Tokenizer:
    _value_factory = set  # type: Callable[[], WeakReferencable]

    def __init__(self) -> None:
        self._value = self._value_factory()

    def create(self) -> Token:
        return Token(self._value)

    def reset(self) -> None:
        del self._value
        self._value = self._value_factory()
