import os
from vcpkg.Install import install_vcpkg
from Logger import logger
from Platform import get_platform, Platform, PLATFORM_TO_STRING, get_arch, Arch

__all__ = ["set_vcpkg_targets"]

_TRIPLET_ARCH_MAPPING = {
    Arch.X86: "x86",
    Arch.X64: "x64",
    Arch.aarch: "arm",
    Arch.aarch64: "arm64"
}

def _get_triplet() -> str:
    logger.info("Determining vcpkg triplet...")
    triplet = None
    platform = get_platform()
    arch = _TRIPLET_ARCH_MAPPING.get(get_arch())
    if arch is None:
        raise RuntimeError(f"Unsupported architecture for vcpkg triplet: {get_arch()}")
    match platform:
        case Platform.WINDOWS:
            triplet = f"{arch}-windows"
        case Platform.LINUX:
            triplet = f"{arch}-linux"
        case Platform.DARWIN:
            triplet = f"{arch}-osx"
        case _:
            raise RuntimeError(f"Unsupported platform for vcpkg triplet: {PLATFORM_TO_STRING[platform]}")
    logger.info(f"Determined vcpkg triplet: {triplet}")
    return triplet


def set_vcpkg_targets() -> list[str]:
    logger.info("Setting vcpkg targets...")
    vcpkg_root = install_vcpkg()
    triplet = _get_triplet()
    cmake_toolchain_file = os.path.join(vcpkg_root, "scripts", "buildsystems", "vcpkg.cmake")
    if not os.path.isfile(cmake_toolchain_file):
        logger.info(f"vcpkg toolchain file not found at {cmake_toolchain_file}; vcpkg integration skipped.")
        return []
    logger.info(f"vcpkg toolchain file found at {cmake_toolchain_file}; setting CMake targets.")
    return [
        f"-DCMAKE_TOOLCHAIN_FILE={cmake_toolchain_file}",
        f"-DVCPKG_TARGET_TRIPLET={triplet}",
    ]
