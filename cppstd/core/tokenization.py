import weakref
from typing import (Generic,
                    TypeVar)

try:
    from typing import Protocol
except ImportError:
    from typing_extensions import Protocol


class WeakReferencable(Protocol):
    __weakref__ = ...


Domain = TypeVar('Domain',
                 bound=WeakReferencable)


class WrappedValue(Generic[Domain]):
    class _ValueFactory:
        __slots__ = '__weakref__',

    __slots__ = 'value',

    def __init__(self) -> None:
        self.value = self._ValueFactory()  # type: Domain


class SharedToken:
    __slots__ = '_value', '_wrapped_value'

    def __init__(self,
                 wrapped_value: WrappedValue[Domain]) -> None:
        self._value = weakref.ref(wrapped_value.value)
        self._wrapped_value = wrapped_value

    @property
    def expired(self) -> bool:
        return self._value() is not self._wrapped_value.value


class WeakToken:
    __slots__ = '_value',

    def __init__(self, value: Domain) -> None:
        self._value = weakref.ref(value)

    @property
    def expired(self) -> bool:
        return self._value() is None


class Tokenizer:
    __slots__ = '_wrapped_value',

    def __init__(self) -> None:
        self._wrapped_value = WrappedValue()

    def create_weak(self) -> WeakToken:
        return WeakToken(self._wrapped_value.value)

    def create_shared(self) -> SharedToken:
        return SharedToken(self._wrapped_value)

    def reset(self) -> None:
        del self._wrapped_value
        self._wrapped_value = WrappedValue()
