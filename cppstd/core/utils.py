from typing import TypeVar

Domain = TypeVar('Domain')


def identity(value: Domain) -> Domain:
    return value
