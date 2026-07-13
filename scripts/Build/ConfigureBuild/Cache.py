import pathlib
import re
import shutil

from Logger import logger
from ParseArgs import args
from Platform import Platform, get_platform

__all__ = ["ensure_compatible_build_cache"]

_WINDOWS_DRIVE_PATTERN = re.compile(r"^[A-Za-z]:[\\/]")


def _read_cache_entries(cache_file: pathlib.Path) -> dict[str, str]:
    entries: dict[str, str] = {}
    for line in cache_file.read_text(encoding="utf-8", errors="ignore").splitlines():
        if not line or line.startswith(("//", "#")) or "=" not in line or ":" not in line:
            continue
        key_with_type, value = line.split("=", 1)
        key = key_with_type.split(":", 1)[0].strip()
        entries[key] = value.strip()
    return entries


def _infer_platform_from_cache(cache_entries: dict[str, str]) -> Platform | None:
    for key in ("CMAKE_SYSTEM_NAME", "CMAKE_HOST_SYSTEM_NAME"):
        value = cache_entries.get(key, "").strip().lower()
        if value == "windows":
            return Platform.WINDOWS
        if value == "linux":
            return Platform.LINUX
        if value == "darwin":
            return Platform.DARWIN

    generator = cache_entries.get("CMAKE_GENERATOR", "").strip().lower()
    if "visual studio" in generator:
        return Platform.WINDOWS

    for key in ("CMAKE_COMMAND", "CMAKE_C_COMPILER", "CMAKE_CXX_COMPILER", "CMAKE_MAKE_PROGRAM"):
        value = cache_entries.get(key, "").strip()
        if not value:
            continue
        normalized_value = value.lower()
        if _WINDOWS_DRIVE_PATTERN.match(value) or normalized_value.endswith(".exe"):
            return Platform.WINDOWS
        if ".app/" in normalized_value or "/library/developer/" in normalized_value:
            return Platform.DARWIN

    return None


def _confirm_delete_incompatible_cache(build_dir: pathlib.Path, cached_platform: Platform) -> bool:
    current_platform = get_platform().name.lower()
    cached_platform_name = cached_platform.name.lower()
    if args.dry_run:
        logger.info(
            f"DRY RUN: delete build cache at {build_dir} because it targets {cached_platform_name} instead of {current_platform}"
        )
        return True
    if args.auto_mode:
        logger.warn(
            f"Build cache targets {cached_platform_name} instead of {current_platform}. Auto mode will delete it."
        )
        return True
    answer = logger.input(
        f"Build cache targets {cached_platform_name} instead of {current_platform}. Delete build cache and reconfigure? (y/n) "
    )
    return answer.strip().lower() in {"y", "yes"}


def _confirm_delete_mismatched_cache(build_dir: pathlib.Path, reason: str) -> bool:
    if args.dry_run:
        logger.info(f"DRY RUN: delete build cache at {build_dir} because {reason}")
        return True
    if args.auto_mode:
        logger.warn(f"Build cache is stale because {reason}. Auto mode will delete it.")
        return True
    answer = logger.input(f"Build cache is stale because {reason}. Delete build cache and reconfigure? (y/n) ")
    return answer.strip().lower() in {"y", "yes"}


def _normalize_cache_path(path: str) -> str:
    normalized = path.strip()
    return normalized[:-1] if normalized.endswith("/") else normalized


def _ensure_matching_cache_paths(build_dir: pathlib.Path, cache_entries: dict[str, str]):
    expected_build_dir = _normalize_cache_path(str(build_dir.resolve()))
    cached_build_dir = _normalize_cache_path(cache_entries.get("CMAKE_CACHEFILE_DIR", ""))
    if cached_build_dir and cached_build_dir != expected_build_dir:
        reason = f"it targets build directory '{cached_build_dir}' instead of '{expected_build_dir}'"
        if not _confirm_delete_mismatched_cache(build_dir, reason):
            raise RuntimeError("Build cache points to a different build directory.")
        if not args.dry_run:
            logger.info(f"Deleting stale build cache at {build_dir}")
            shutil.rmtree(build_dir, ignore_errors=True)
        return

    expected_source_dir = _normalize_cache_path(str(build_dir.parent.resolve()))
    cached_source_dir = _normalize_cache_path(cache_entries.get("CMAKE_HOME_DIRECTORY", ""))
    if cached_source_dir and cached_source_dir != expected_source_dir:
        reason = f"it targets source directory '{cached_source_dir}' instead of '{expected_source_dir}'"
        if not _confirm_delete_mismatched_cache(build_dir, reason):
            raise RuntimeError("Build cache points to a different source directory.")
        if not args.dry_run:
            logger.info(f"Deleting stale build cache at {build_dir}")
            shutil.rmtree(build_dir, ignore_errors=True)


def _toolchain_files_used_by_cmake(build_dir: pathlib.Path) -> set[str]:
    """Return toolchain files recorded in CMake's generated system files."""
    toolchain_files: set[str] = set()
    include_pattern = re.compile(r'^include\("(.+vcpkg\.cmake)"\)$')
    for system_file in build_dir.glob("CMakeFiles/*/CMakeSystem.cmake"):
        for line in system_file.read_text(encoding="utf-8", errors="ignore").splitlines():
            match = include_pattern.match(line.strip())
            if match:
                toolchain_files.add(_normalize_cache_path(match.group(1)))
    return toolchain_files


def _ensure_matching_toolchain_file(
    build_dir: pathlib.Path, cache_entries: dict[str, str], expected_toolchain_file: str | None
):
    if expected_toolchain_file is None:
        return

    expected = _normalize_cache_path(str(pathlib.Path(expected_toolchain_file).resolve()))
    cached = _normalize_cache_path(cache_entries.get("CMAKE_TOOLCHAIN_FILE", ""))
    system_toolchains = _toolchain_files_used_by_cmake(build_dir)
    mismatches = [path for path in (cached, *system_toolchains) if path and path != expected]
    if not mismatches:
        return

    reason = f"it uses vcpkg toolchain '{mismatches[0]}' instead of '{expected}'"
    if not _confirm_delete_mismatched_cache(build_dir, reason):
        raise RuntimeError("Build cache uses a different vcpkg toolchain.")
    if not args.dry_run:
        logger.info(f"Deleting stale build cache at {build_dir}")
        shutil.rmtree(build_dir, ignore_errors=True)


def ensure_compatible_build_cache(build_dir: pathlib.Path, expected_toolchain_file: str | None = None):
    logger.info(f"Ensuring compatible build cache at {build_dir}")
    cache_file = build_dir / "CMakeCache.txt"
    if not cache_file.is_file():
        return

    cache_entries = _read_cache_entries(cache_file)
    _ensure_matching_cache_paths(build_dir, cache_entries)
    if not cache_file.is_file():
        return

    _ensure_matching_toolchain_file(build_dir, cache_entries, expected_toolchain_file)
    if not cache_file.is_file():
        return

    cached_platform = _infer_platform_from_cache(cache_entries)
    if cached_platform is None or cached_platform == get_platform():
        return

    if not _confirm_delete_incompatible_cache(build_dir, cached_platform):
        raise RuntimeError("Build cache is incompatible with the current platform.")

    if args.dry_run:
        return
    logger.info(f"Deleting incompatible build cache at {build_dir}")
    shutil.rmtree(build_dir, ignore_errors=True)
