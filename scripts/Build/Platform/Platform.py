import platform
from enum import Enum, auto
from Logger import logger

__all__ = ["Platform", "get_platform", "PLATFORM_REQUIRED_TOOLS", "PLATFORM_TO_STRING", "is_unix"]

__unix = None
__cur_platform: Platform | None = None

class Platform(Enum):
    WINDOWS = auto()
    LINUX = auto()
    DARWIN = auto()

__GENERAL_REQUIRED_TOOLS = [
    {"git"},
    {"cmake"},
]

__UNIX_REQUIRED_TOOLS = [
    {"ninja", "make"},
]

PLATFORM_REQUIRED_TOOLS = {
    Platform.WINDOWS:
    __GENERAL_REQUIRED_TOOLS + [
        {"ninja", "make", "msbuild"},

    ],
    Platform.LINUX:
    __GENERAL_REQUIRED_TOOLS + __UNIX_REQUIRED_TOOLS + [

    ],
    Platform.DARWIN:
    __GENERAL_REQUIRED_TOOLS + __UNIX_REQUIRED_TOOLS + [

    ],
}

PLATFORM_TO_STRING = {
    Platform.WINDOWS: "Windows",
    Platform.LINUX: "Linux",
    Platform.DARWIN: "Darwin"
}

def get_platform() -> Platform:
    global __cur_platform
    if __cur_platform is not None:
        return __cur_platform
    logger.info("Detecting platform...")
    platform_str = platform.system()
    if platform_str == "Windows":
        __cur_platform = Platform.WINDOWS
    elif platform_str == "Linux":
        __cur_platform = Platform.LINUX
    elif platform_str == "Darwin":
        __cur_platform = Platform.DARWIN
    else:
        raise RuntimeError(f"Unsupported platform: {platform_str}")
    logger.info(f"Detected platform: {PLATFORM_TO_STRING[__cur_platform]}")
    return __cur_platform

def is_unix() -> bool:
    global __unix
    if __unix is not None:
        return __unix
    logger.info("Checking if platform is Unix-like...")
    platform = get_platform()
    if platform in (Platform.LINUX, Platform.DARWIN):
        __unix = True
        logger.info("Platform is Unix-like.")
    else:
        __unix = False
        logger.info("Platform is not Unix-like.")
    return __unix
