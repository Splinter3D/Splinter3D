import os
import pathlib
import shutil
import subprocess
from Logger import logger
from ParseArgs import args
from enum import Enum, auto
from Platform import Platform, get_platform

__all__ = ["choose_build_system", "run_build_tool", "BuildSystem", "BUILD_SYSTEM_TO_STRING"]

class BuildSystem(Enum):
    NINJA = auto()
    MAKE = auto()
    VISUAL_STUDIO = auto()

BUILD_SYSTEM_TO_STRING = {
    BuildSystem.NINJA: "Ninja",
    BuildSystem.MAKE: "Unix Makefiles",
    BuildSystem.VISUAL_STUDIO: "Visual Studio 17 2022",
}

STRING_TO_BUILD_SYSTEM = {
    "ninja": BuildSystem.NINJA,
    "unix makefiles": BuildSystem.MAKE,
}

def choose_build_system() -> BuildSystem:
    logger.info("Choosing build system...")
    if args.generator:
        normalized_generator = args.generator.strip().lower()
        if normalized_generator in STRING_TO_BUILD_SYSTEM:
            selected = STRING_TO_BUILD_SYSTEM[normalized_generator]
            logger.info(f"Using user-selected generator: {BUILD_SYSTEM_TO_STRING[selected]}")
            return selected
        if "visual studio" in normalized_generator:
            logger.info("Using user-selected generator: Visual Studio")
            return BuildSystem.VISUAL_STUDIO
        logger.error(f"Unsupported generator: {args.generator}")
        raise RuntimeError(f"Unsupported generator: {args.generator}")

    env_generator = os.getenv("CMAKE_GENERATOR", "").strip().lower()
    if env_generator:
        if env_generator in STRING_TO_BUILD_SYSTEM:
            selected = STRING_TO_BUILD_SYSTEM[env_generator]
            logger.info(f"Using CMAKE_GENERATOR={BUILD_SYSTEM_TO_STRING[selected]}")
            return selected
        if "visual studio" in env_generator:
            logger.info("Using CMAKE_GENERATOR=Visual Studio")
            return BuildSystem.VISUAL_STUDIO

    if get_platform() == Platform.WINDOWS:
        logger.info("Defaulting to Visual Studio generator on Windows.")
        return BuildSystem.VISUAL_STUDIO

    if shutil.which("ninja"):
        logger.info("Ninja build system found.")
        return BuildSystem.NINJA
    if shutil.which("make"):
        logger.info("Make build system found.")
        return BuildSystem.MAKE

    logger.error("No suitable build system found. Please install Ninja or Make.")
    raise RuntimeError("No suitable build system found. Please install Ninja or Make.")

def _is_multi_config(build_system: BuildSystem) -> bool:
    return build_system == BuildSystem.VISUAL_STUDIO

def _detect_build_system_from_cache(build_dir: pathlib.Path) -> BuildSystem:
    cache_file = build_dir / "CMakeCache.txt"
    if not cache_file.exists():
        logger.error("Missing CMakeCache.txt in build directory.")
        raise RuntimeError("Missing CMakeCache.txt in build directory.")

    generator_value = ""
    for line in cache_file.read_text(encoding="utf-8", errors="ignore").splitlines():
        if line.startswith("CMAKE_GENERATOR:INTERNAL="):
            generator_value = line.split("=", 1)[1].strip()
            break

    normalized = generator_value.lower()
    if normalized in STRING_TO_BUILD_SYSTEM:
        return STRING_TO_BUILD_SYSTEM[normalized]
    if "visual studio" in normalized:
        return BuildSystem.VISUAL_STUDIO

    logger.error(f"Unsupported configured generator: {generator_value}")
    raise RuntimeError(f"Unsupported configured generator: {generator_value}")

def run_build_tool(build_system: BuildSystem | None, target: str, build_type: str):
    if build_system is None:
        build_system = _detect_build_system_from_cache(pathlib.Path.cwd())

    command = [
        "cmake",
        "--build",
        ".",
        "--target",
        target,
        "--parallel",
        str(os.cpu_count() or 1),
    ]
    if _is_multi_config(build_system):
        command.extend(["--config", build_type])

    logger.info(f"Running build command: {' '.join(command)}")
    if args.dry_run:
        logger.info(f"DRY RUN: {' '.join(command)}")
        return
    try:
        subprocess.run(
            command,
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
    except subprocess.CalledProcessError as e:
        stdout = (e.stdout or "").strip()
        stderr = (e.stderr or "").strip()
        if stdout:
            logger.error(f"Build stdout:\n{stdout}")
        if stderr:
            logger.error(f"Build stderr:\n{stderr}")
        logger.error(f"Build command failed: {e}")
        raise RuntimeError(f"Build command failed: {e}")
    logger.info(f"Succesfully built {target}.")
