import inspect
import os
import sys
from dataclasses import dataclass
from datetime import datetime

__all__ = ["logger"]

class Logger:
    @dataclass
    class Config:
        debug: bool = False

    @dataclass(frozen=True)
    class Level:
        DEBUG = "DEBUG"
        INFO = "INFO"
        WARN = "WARN"
        ERROR = "ERROR"

    COLOR_RESET = "\033[0m"
    COLOR_BOLD = "\033[1m"
    COLOR_DEBUG = "\033[38;5;188m"
    COLOR_INFO = "\033[38;5;183m"
    COLOR_WARN = "\033[38;5;216m"
    COLOR_ERROR = "\033[38;5;203m"
    COLOR_CONTEXT = "\033[38;5;138m"

    @classmethod
    def get_frame_info(cls):
        frame = inspect.currentframe()
        if frame is not None and frame.f_back is not None and frame.f_back.f_back is not None:
            return frame.f_back.f_back.f_code.co_filename, frame.f_back.f_back.f_lineno
        return None, None

    @classmethod
    def input(cls, prompt: str) -> str:
        filename, lineno = cls.get_frame_info()
        cls._emit(prompt, cls.Level.INFO, filename, lineno)
        return input()

    @classmethod
    def debug(cls, message: str):
        if (cls.Config.debug is not True):
            return
        filename, lineno = cls.get_frame_info()
        cls._emit(message, cls.Level.DEBUG, filename, lineno)

    @classmethod
    def info(cls, message: str):
        filename, lineno = cls.get_frame_info()
        cls._emit(message, cls.Level.INFO, filename, lineno)

    @classmethod
    def warn(cls, message: str):
        filename, lineno = cls.get_frame_info()
        cls._emit(message, cls.Level.WARN, filename, lineno)

    @classmethod
    def error(cls, message: str):
        filename, lineno = cls.get_frame_info()
        cls._emit(message, cls.Level.ERROR, filename, lineno)

    @classmethod
    def _emit(cls, message: str, level: str, file: str | None, line: int | None):
        now = datetime.now()
        timestamp = now.strftime("%H:%M:%S")

        filename = os.path.basename(file) if file is not None else "(None)"
        line_str = str(line) if line is not None and file is not None else "(None)"
        color = cls._level_to_color(level)

        log_line = [f"{cls.COLOR_BOLD}{color}[{level}]\t"]
        if (cls.Config.debug is True):
            log_line.append(f"{cls.COLOR_CONTEXT}{timestamp} {filename}:{line_str}")
        log_line.append(f"{cls.COLOR_RESET} {cls.COLOR_BOLD}{color}{message}{cls.COLOR_RESET}")

        print("".join(log_line), file=sys.stdout)

    @classmethod
    def _level_to_color(cls, level: str) -> str:
        if level == cls.Level.DEBUG:
            return cls.COLOR_DEBUG

        elif level == cls.Level.INFO:
            return cls.COLOR_INFO

        elif level == cls.Level.WARN:
            return cls.COLOR_WARN

        elif level == cls.Level.ERROR:
            return cls.COLOR_ERROR

        return cls.COLOR_RESET

    @classmethod
    def set_debug(cls, debug: bool):
        cls.Config.debug = debug

logger = Logger()
