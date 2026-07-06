import os
import subprocess
from Logger import logger

__all__ = ["detect_vcpkg"]

def detect_vcpkg() -> str | None:
    logger.info("Detecting vcpkg installation...")
    try:
        subprocess.run(["vcpkg", "--version"], check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        vcpkg_executable = subprocess.run(["which", "vcpkg"], check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE).stdout.decode().strip()
        logger.info(f"Found vcpkg executable at: {vcpkg_executable}")
        return vcpkg_executable
    except (subprocess.CalledProcessError, FileNotFoundError):
        logger.info("vcpkg not found in PATH")

    path_candidates = [
        os.getcwd(),
        os.getenv("HOME") if os.name != "nt" else os.getenv("USERPROFILE"),
    ]

    vcpkg_root = os.getenv("VCPKG_ROOT")
    if vcpkg_root:
        path_candidates.append(vcpkg_root)

    for path in path_candidates:
        vcpkg_path = os.path.join(path, "vcpkg")
        if os.path.isdir(vcpkg_path):
            try:
                vcpkg_executable = os.path.join(vcpkg_path, "vcpkg")
                logger.info(f"Found vcpkg executable at: {vcpkg_executable}")
                subprocess.run([vcpkg_executable, "--version"], check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
                return vcpkg_executable
            except (subprocess.CalledProcessError, FileNotFoundError):
                continue

    logger.info(f"vcpkg executable not found.")
    return None
