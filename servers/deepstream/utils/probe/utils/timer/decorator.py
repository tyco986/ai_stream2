import functools
import logging
import time


class timer:
    """Decorator that records wall time of a call in milliseconds.

    Usage:
        @timer
        def f(...): ...

        @timer(result_key="draw_ms")
        def f(...): ...  # injects round(ms, 2) into returned dict

    After a call, read ``last_ms`` on the decorator or on the instance
    (for methods: ``drawer.last_ms``).
    """

    def __init__(self, func=None, *, name=None, logger=None, level=logging.DEBUG, result_key=None):
        self.func = None
        self.name = name
        self.logger = logger
        self.level = level
        self.result_key = result_key
        self.last_ms = 0.0
        if func is not None:
            self.wrap(func)

    def wrap(self, func):
        self.func = func
        if self.name is None:
            self.name = func.__qualname__
        functools.update_wrapper(self, func)
        return self

    def __call__(self, *args, **kwargs):
        result = None
        if self.func is None:
            result = self.wrap(args[0])
        else:
            start = time.perf_counter()
            result = self.func(*args, **kwargs)
            elapsed_ms = (time.perf_counter() - start) * 1000
            self.last_ms = elapsed_ms
            if args and not isinstance(args[0], type):
                args[0].last_ms = elapsed_ms
            if self.result_key is not None and isinstance(result, dict):
                result[self.result_key] = round(elapsed_ms, 2)
            if self.logger is not None:
                self.logger.log(self.level, "%s: %.2f ms", self.name, elapsed_ms)
        return result

    def __get__(self, obj, objtype=None):
        bound = self
        if obj is not None:
            bound = functools.partial(self.__call__, obj)
        return bound
