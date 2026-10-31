"""Locates an existing vcpkg installation, either on PATH or in common install locations,
without installing anything (see vcpkg/Install.py for that)."""

import os
import shutil
import subprocess
from Logger import logger
from Platform import get_platform, Platform
from vcpkg.Validation import expected_vcpkg_executable_name, inspect_vcpkg_root

__all__ = ["detect_vcpkg"]

def detect_vcpkg() -> str | None:
    path_candidates = []
    logger.info("Detecting vcpkg installation...")
    executable_name = expected_vcpkg_executable_name()
    try:
        subprocess.run([executable_name, "--version"], check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        vcpkg_executable = shutil.which(executable_name)
        if not vcpkg_executable:
            raise FileNotFoundError(executable_name)
        logger.info(f"Found vcpkg executable at: {vcpkg_executable}")
        return vcpkg_executable
    except (subprocess.CalledProcessError, FileNotFoundError, OSError):
        logger.info("vcpkg not found in PATH")

    # Search order mirrors where each platform's install script places vcpkg:
    # Windows CI (install.ps1) clones it under %USERPROFILE%, while Unix installs
    # (vcpkg/Install.py) default to the current working directory.
    if get_platform() == Platform.WINDOWS:
        path_candidates = [
            os.getenv("USERPROFILE"),
            os.getcwd(),
        ]
    else:
        path_candidates = [
            os.getcwd(),
            os.getenv("HOME"),
        ]

    vcpkg_root = os.getenv("VCPKG_ROOT")
    if vcpkg_root:
        path_candidates.append(vcpkg_root)

    for path in path_candidates:
        if not path:
            continue
        vcpkg_path = os.path.join(path, "vcpkg")
        if os.path.isdir(vcpkg_path):
            installation_status = inspect_vcpkg_root(vcpkg_path)
            if installation_status.executable_path is None:
                continue
            try:
                vcpkg_executable = installation_status.executable_path
                logger.info(f"Found vcpkg executable at: {vcpkg_executable}")
                subprocess.run([vcpkg_executable, "--version"], check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
                return vcpkg_executable
            except (subprocess.CalledProcessError, FileNotFoundError, OSError):
                continue

    logger.info(f"vcpkg executable not found.")
    return None
