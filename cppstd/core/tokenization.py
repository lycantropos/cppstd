import weakref

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
    class _ValueFactory:
        __slots__ = '__weakref__',

    __slots__ = '_value',

    def __init__(self) -> None:
        self._value = self._ValueFactory()  # type: WeakReferencable

    def create(self) -> Token:
        return Token(self._value)

    def reset(self) -> None:
        del self._value
        self._value = self._ValueFactory()
