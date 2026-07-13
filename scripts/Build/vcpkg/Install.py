import os
import pathlib
import shutil
import subprocess
from Logger import logger
from ParseArgs import args
from Platform import is_unix
from vcpkg.Detect import detect_vcpkg
from vcpkg.Validation import expected_vcpkg_executable_name, inspect_vcpkg_root

__all__ = ["install_vcpkg"]

def _looks_like_vcpkg_root(path: str) -> bool:
    normalized = os.path.abspath(path)
    return any(
        os.path.exists(candidate)
        for candidate in (
            os.path.join(normalized, "scripts", "buildsystems", "vcpkg.cmake"),
            os.path.join(normalized, "bootstrap-vcpkg.sh"),
            os.path.join(normalized, "bootstrap-vcpkg.bat"),
            os.path.join(normalized, "ports"),
        )
    )

def _normalize_vcpkg_root(vcpkg_path: str) -> str:
    normalized = os.path.abspath(vcpkg_path)
    if os.path.isfile(normalized):
        candidate_root = os.path.dirname(normalized)
        if _looks_like_vcpkg_root(candidate_root):
            normalized = candidate_root
    return normalized


def _is_empty_directory(path: str) -> bool:
    return os.path.isdir(path) and not os.listdir(path)


def _resolve_vcpkg_path(vcpkg_path: str) -> str:
    """Accept either a vcpkg root or an existing directory to contain it."""
    normalized = _normalize_vcpkg_root(vcpkg_path)
    if (
        args.vcpkg_path
        and os.path.isdir(normalized)
        and not _looks_like_vcpkg_root(normalized)
        and os.path.basename(normalized).lower() != "vcpkg"
    ):
        return os.path.join(normalized, "vcpkg")
    return normalized


def _get_vcpkg_path() -> str:
    default_path = os.path.join(os.getcwd(), "vcpkg")
    vcpkg_root = os.getenv("VCPKG_ROOT")
    if vcpkg_root:
        default_path = vcpkg_root if vcpkg_root.endswith("vcpkg") else os.path.join(vcpkg_root, "vcpkg")
    return default_path

def _reset_vcpkg_root(vcpkg_path: str):
    if os.path.isdir(vcpkg_path):
        logger.info(f"Removing vcpkg directory: {vcpkg_path}")
        shutil.rmtree(vcpkg_path)


def _should_reinstall_incompatible_vcpkg(vcpkg_root: str) -> bool:
    expected_executable = expected_vcpkg_executable_name()
    if args.dry_run:
        logger.info(f"DRY RUN: reinstall incompatible vcpkg at {vcpkg_root} to provide {expected_executable}")
        return True
    if args.auto_mode:
        logger.warn(f"Incompatible vcpkg installation detected at {vcpkg_root}. Auto mode will reinstall it.")
        return True
    answer = logger.input(
        f"vcpkg at {vcpkg_root} is not installed for this OS. Reinstall it for the current platform? (y/n) "
    )
    return answer.strip().lower() in {"y", "yes"}


def _ensure_current_platform_vcpkg(vcpkg_root: str) -> str | None:
    installation_status = inspect_vcpkg_root(vcpkg_root)
    if installation_status.matches_current_platform:
        logger.info(f"vcpkg is already installed at: {installation_status.root}")
        return installation_status.root
    if not installation_status.wrong_platform:
        return None
    if not _should_reinstall_incompatible_vcpkg(installation_status.root):
        raise RuntimeError("vcpkg installation is incompatible with the current platform.")
    if args.dry_run:
        return installation_status.root
    _reset_vcpkg_root(installation_status.root)
    return None

def _clone(vcpkg_path: str):
    logger.info(f"Cloning vcpkg into: {vcpkg_path}")
    pathlib.Path(vcpkg_path).parent.mkdir(exist_ok=True, parents=True)
    try:
        subprocess.run(["git", "clone", "https://github.com/microsoft/vcpkg.git", vcpkg_path], check=True)
    except subprocess.CalledProcessError as e:
        logger.error(f"Failed to clone vcpkg: {e}")
        raise RuntimeError(f"Failed to clone vcpkg: {e}")

def _bootstrap(vcpkg_path: str):
    logger.info(f"Bootstrapping vcpkg...")
    if is_unix():
        bootstrap_script = os.path.join(vcpkg_path, "bootstrap-vcpkg.sh")
    else:
        bootstrap_script = os.path.join(vcpkg_path, "bootstrap-vcpkg.bat")
    logger.info(f"Bootstrapping vcpkg using: {bootstrap_script}")
    if not os.path.isfile(bootstrap_script):
        logger.error(f"Bootstrap script not found: {bootstrap_script}")
        raise RuntimeError(f"Bootstrap script not found: {bootstrap_script}")
    try:
        subprocess.run(
            [bootstrap_script],
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
    except subprocess.CalledProcessError as e:
        stdout = (e.stdout or "").strip()
        stderr = (e.stderr or "").strip()
        if stdout:
            logger.error(f"vcpkg bootstrap stdout:\n{stdout}")
        if stderr:
            logger.error(f"vcpkg bootstrap stderr:\n{stderr}")
        logger.error(f"Failed to bootstrap vcpkg: {e}")
        raise RuntimeError(f"Failed to bootstrap vcpkg: {e}")

def install_vcpkg() -> str:
    vcpkg_path = args.vcpkg_path if args.vcpkg_path else _get_vcpkg_path()
    vcpkg_path = _resolve_vcpkg_path(vcpkg_path)
    reinstall = args.reinstall_vcpkg or args.reinstall_dependencies
    if args.skip_vcpkg:
        logger.info("Skipping vcpkg installation and configuration as per the provided argument.")
        return _normalize_vcpkg_root(vcpkg_path)
    if reinstall:
        vcpkg_path = _normalize_vcpkg_root(vcpkg_path)
        if args.dry_run:
            logger.info(f"DRY RUN: reinstall vcpkg at {vcpkg_path}")
            return vcpkg_path
        _reset_vcpkg_root(vcpkg_path)
    else:
        configured_root = _normalize_vcpkg_root(vcpkg_path)
        configured_vcpkg = _ensure_current_platform_vcpkg(configured_root)
        if configured_vcpkg is not None:
            return configured_vcpkg
        if os.path.isdir(configured_root) and _looks_like_vcpkg_root(configured_root):
            logger.info(f"Using configured vcpkg root: {configured_root}")
            vcpkg_path = configured_root
        elif not args.vcpkg_path:
            detected_vcpkg = detect_vcpkg()
            if detected_vcpkg:
                vcpkg_root = _normalize_vcpkg_root(detected_vcpkg)
                if _looks_like_vcpkg_root(vcpkg_root):
                    detected_root = _ensure_current_platform_vcpkg(vcpkg_root)
                    if detected_root is not None:
                        return detected_root
                else:
                    logger.info(
                        f"Ignoring vcpkg executable without a usable root for CMake integration: {detected_vcpkg}"
                    )
        if args.dry_run:
            logger.info(f"DRY RUN: ensure vcpkg is available at {configured_root}")
            return configured_root
    if args.auto_mode is True or args.vcpkg_path or reinstall:
        install = "y"
    else:
        install = logger.input("vcpkg is not installed. Would you like to install it? (y/n) ")
    if install.lower() != "y":
        logger.error("vcpkg installation aborted by user.")
        raise RuntimeError("vcpkg installation aborted by user.")
    if not os.path.exists(vcpkg_path) or _is_empty_directory(vcpkg_path):
        if args.auto_mode is not True and not args.vcpkg_path:
            vcpkg_path = logger.input(f"vcpkg not found. Where would you like to clone it? (default: {vcpkg_path}) ") or vcpkg_path
        _clone(vcpkg_path)
    vcpkg_path = _normalize_vcpkg_root(vcpkg_path)
    if not os.path.isdir(vcpkg_path) or not _looks_like_vcpkg_root(vcpkg_path):
        message = f"vcpkg path is not a vcpkg checkout: {vcpkg_path}"
        logger.error(message)
        raise RuntimeError(message)
    _bootstrap(vcpkg_path)
    executable_path = os.path.join(vcpkg_path, expected_vcpkg_executable_name())
    if not os.path.isfile(executable_path) or not os.access(executable_path, os.X_OK):
        logger.error(f"vcpkg executable not found at: {executable_path}. Please check the path and try again.")
        raise RuntimeError(f"vcpkg executable not found at: {executable_path}. Please check the path and try again.")
    logger.info(f"vcpkg installed successfully at: {executable_path}")
    return vcpkg_path
