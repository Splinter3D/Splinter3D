import os
import subprocess
from Logger import logger
from dataclasses import dataclass
from Platform import Platform, get_platform

__all__ = [
    "VcpkgInstallationStatus",
    "expected_vcpkg_executable_name",
    "inspect_vcpkg_root",
]


@dataclass(frozen=True)
class VcpkgInstallationStatus:
    root: str
    executable_path: str | None
    matches_current_platform: bool
    wrong_platform: bool


def expected_vcpkg_executable_name() -> str:
    return "vcpkg.exe" if get_platform() == Platform.WINDOWS else "vcpkg"


def _candidate_executables(vcpkg_root: str) -> tuple[str, ...]:
    return (
        os.path.join(vcpkg_root, "vcpkg"),
        os.path.join(vcpkg_root, "vcpkg.exe"),
    )


def _is_exec_format_error(error: OSError) -> bool:
    return error.errno == 8 or getattr(error, "winerror", None) == 193


def _probe_executable(path: str) -> tuple[bool, bool]:
    logger.info(f"Probing vcpkg executable: {path}")
    if not os.path.isfile(path):
        return False, False
    try:
        process = subprocess.Popen(
            [path, "--version"],
            stdin=subprocess.DEVNULL,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
    except OSError as error:
        return False, _is_exec_format_error(error)
    try:
        if process.poll() is None:
            process.terminate()
            try:
                process.wait(timeout=1)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()
        return True, False
    finally:
        if process.poll() is None:
            process.kill()
            process.wait()


def inspect_vcpkg_root(vcpkg_root: str) -> VcpkgInstallationStatus:
    logger.info(f"Inspecting vcpkg installation: {vcpkg_root}")
    normalized_root = os.path.abspath(vcpkg_root)
    matching_executable: str | None = None
    found_incompatible_binary = False

    for executable_path in _candidate_executables(normalized_root):
        executable_runs, wrong_platform = _probe_executable(executable_path)
        if executable_runs:
            matching_executable = executable_path
            break
        if wrong_platform:
            found_incompatible_binary = True

    return VcpkgInstallationStatus(
        root=normalized_root,
        executable_path=matching_executable,
        matches_current_platform=matching_executable is not None,
        wrong_platform=matching_executable is None and found_incompatible_binary,
    )
