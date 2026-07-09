import platform
from enum import Enum, auto
from Logger import logger

__all__ = ["Arch", "get_arch", "ARCH_TO_STRING"]

class Arch(Enum):
    X86 = auto()
    X64 = auto()
    aarch = auto()
    aarch64 = auto()

_cur_arch: Arch | None = None

ARCH_TO_STRING = {
    Arch.X86: "x86",
    Arch.X64: "x64",
    Arch.aarch: "aarch",
    Arch.aarch64: "aarch64"
}

def get_arch() -> Arch:
    global _cur_arch
    if _cur_arch is not None:
        return _cur_arch
    logger.info("Detecting architecture...")
    arch_str = platform.machine().lower()
    logger.debug(f"Platform machine string: {arch_str}")
    if arch_str in ["x86", "i386", "i686"]:
        _cur_arch = Arch.X86
    elif arch_str in ["x86_64", "amd64"]:
        _cur_arch = Arch.X64
    elif arch_str in ["arm", "armv7l"]:
        _cur_arch = Arch.aarch
    elif arch_str in ["aarch64", "arm64"]:
        _cur_arch = Arch.aarch64
    else:
        raise RuntimeError(f"Unsupported architecture: {arch_str}")
    logger.info(f"Detected architecture: {ARCH_TO_STRING[_cur_arch]}")
    return _cur_arch
