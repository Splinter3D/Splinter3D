import platform
from enum import Enum, auto
from Logger import logger

__all__ = ["Platform", "get_platform", "PLATFORM_REQUIRED_TOOLS", "PLATFORM_TO_STRING", "is_unix"]

class Platform(Enum):
    WINDOWS = auto()
    LINUX = auto()
    DARWIN = auto()

_unix = None
_cur_platform: Platform | None = None

_GENERAL_REQUIRED_TOOLS = [
    {"git"},
    {"cmake"},
]

_UNIX_REQUIRED_TOOLS = [
    {"g++", "clang++"},
    {"ninja", "make"},
]

PLATFORM_REQUIRED_TOOLS = {
    Platform.WINDOWS:
    _GENERAL_REQUIRED_TOOLS + [
        {"ninja", "make", "msbuild"},
    ],
    Platform.LINUX:
    _GENERAL_REQUIRED_TOOLS + _UNIX_REQUIRED_TOOLS + [

    ],
    Platform.DARWIN:
    _GENERAL_REQUIRED_TOOLS + _UNIX_REQUIRED_TOOLS + [

    ],
}

PLATFORM_TO_STRING = {
    Platform.WINDOWS: "Windows",
    Platform.LINUX: "Linux",
    Platform.DARWIN: "Darwin"
}

def get_platform() -> Platform:
    global _cur_platform
    if _cur_platform is not None:
        return _cur_platform
    logger.info("Detecting platform...")
    platform_str = platform.system()
    if platform_str == "Windows":
        _cur_platform = Platform.WINDOWS
    elif platform_str == "Linux":
        _cur_platform = Platform.LINUX
    elif platform_str == "Darwin":
        _cur_platform = Platform.DARWIN
    else:
        raise RuntimeError(f"Unsupported platform: {platform_str}")
    logger.info(f"Detected platform: {PLATFORM_TO_STRING[_cur_platform]}")
    return _cur_platform

def is_unix() -> bool:
    global _unix
    if _unix is not None:
        return _unix
    logger.info("Checking if platform is Unix-like...")
    platform = get_platform()
    if platform in (Platform.LINUX, Platform.DARWIN):
        _unix = True
        logger.info("Platform is Unix-like.")
    else:
        _unix = False
        logger.info("Platform is not Unix-like.")
    return _unix
