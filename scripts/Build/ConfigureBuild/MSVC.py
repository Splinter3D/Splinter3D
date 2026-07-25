import os
import subprocess
import tempfile

from Logger import logger
from Platform import Platform, get_platform, get_arch, Arch

__all__ = ["ensure_msvc_environment"]

_ARCH_TO_VSDEV = {
    Arch.X86: "x86",
    Arch.X64: "x64",
    Arch.aarch: "arm",
    Arch.aarch64: "arm64",
}


def _detect_host_arch() -> str:
    arch = os.environ.get("VSCMD_ARG_HOST_ARCH", "").lower()
    if arch in {"x86", "x64", "arm", "arm64"}:
        return arch

    wow64_arch = os.environ.get("PROCESSOR_ARCHITEW6432", "").lower()
    if wow64_arch in {"amd64", "x86", "arm64"}:
        return {"amd64": "x64", "x86": "x86", "arm64": "arm64"}[wow64_arch]

    process_arch = os.environ.get("PROCESSOR_ARCHITECTURE", "").lower()
    if process_arch in {"amd64", "x86", "arm64"}:
        return {"amd64": "x64", "x86": "x86", "arm64": "arm64"}[process_arch]

    return "x64"


def _find_vswhere() -> str | None:
    program_files_x86 = os.environ.get("ProgramFiles(x86)")
    if program_files_x86:
        candidate = os.path.join(program_files_x86, "Microsoft Visual Studio", "Installer", "vswhere.exe")
        if os.path.isfile(candidate):
            return candidate

    result = subprocess.run(
        ["where", "vswhere"],
        check=False,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    for line in result.stdout.splitlines():
        candidate = line.strip()
        if candidate:
            return candidate
    return None


def _query_installation_path(vswhere: str, target_arch: str) -> str:
    component_queries = {
        "arm64": [
            "Microsoft.VisualStudio.Component.VC.Tools.ARM64",
            "Microsoft.VisualStudio.Component.VC.ATL.ARM64",
        ],
        "arm": [
            "Microsoft.VisualStudio.Component.VC.Tools.ARM64",
        ],
        "x64": [
            "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
        ],
        "x86": [
            "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
        ],
    }

    queries: list[list[str]] = []
    for component in component_queries.get(target_arch, []):
        queries.append(
            [
                vswhere,
                "-latest",
                "-products",
                "*",
                "-requires",
                component,
                "-property",
                "installationPath",
            ]
        )

    queries.append(
        [
            vswhere,
            "-latest",
            "-products",
            "*",
            "-requires",
            "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
            "-property",
            "installationPath",
        ]
    )
    queries.append(
        [
            vswhere,
            "-latest",
            "-products",
            "*",
            "-property",
            "installationPath",
        ]
    )

    for query in queries:
        result = subprocess.run(
            query,
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        installation_path = result.stdout.strip()
        if installation_path:
            return installation_path

    return ""


def _find_vsdevcmd(target_arch: str) -> str:
    vswhere = _find_vswhere()
    if not vswhere:
        raise RuntimeError("vswhere.exe not found. Install Visual Studio Build Tools or run from a Developer Command Prompt.")

    installation_path = _query_installation_path(vswhere, target_arch)
    if not installation_path:
        raise RuntimeError(f"Visual Studio with C++ build tools for target architecture '{target_arch}' was not found.")

    vsdevcmd = os.path.join(installation_path, "Common7", "Tools", "VsDevCmd.bat")
    if not os.path.isfile(vsdevcmd):
        raise RuntimeError(f"VsDevCmd.bat not found at: {vsdevcmd}")
    return vsdevcmd


def ensure_msvc_environment() -> None:
    if get_platform() != Platform.WINDOWS:
        return

    target_arch = _ARCH_TO_VSDEV.get(get_arch())
    if target_arch is None:
        raise RuntimeError(f"Unsupported MSVC target architecture: {get_arch()}")

    current_arch = os.environ.get("VSCMD_ARG_TGT_ARCH", "").lower()
    if current_arch == target_arch and os.environ.get("VCINSTALLDIR"):
        logger.info(f"MSVC environment already loaded for target architecture: {target_arch}")
        return

    vsdevcmd = _find_vsdevcmd(target_arch)
    logger.info(f"Importing MSVC environment for target architecture: {target_arch}")
    logger.debug(f"Using VsDevCmd at: {vsdevcmd}")
    host_candidates = [_detect_host_arch()]
    if target_arch not in host_candidates:
        host_candidates.append(target_arch)
    for fallback in ("x64", "arm64", "x86"):
        if fallback not in host_candidates:
            host_candidates.append(fallback)

    result = None
    last_error: Exception | None = None
    for host_arch in host_candidates:
        logger.debug(f"Trying VsDevCmd with host_arch={host_arch}, target_arch={target_arch}")
        wrapper_contents = "\r\n".join(
            [
                "@echo off",
                f'call "{vsdevcmd}" -no_logo -arch={target_arch} -host_arch={host_arch}',
                "if errorlevel 1 exit /b %errorlevel%",
                "set",
                "",
            ]
        )
        try:
            with tempfile.NamedTemporaryFile("w", suffix=".bat", delete=False, encoding="utf-8", newline="") as wrapper:
                wrapper.write(wrapper_contents)
                wrapper_path = wrapper.name
            try:
                result = subprocess.run(
                    ["cmd", "/d", "/c", wrapper_path],
                    check=True,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                    text=True,
                    timeout=180,
                )
            finally:
                try:
                    os.unlink(wrapper_path)
                except OSError:
                    pass
            break
        except subprocess.TimeoutExpired as exc:
            last_error = RuntimeError(
                f"Timed out while importing MSVC environment for architecture '{target_arch}' "
                f"with host architecture '{host_arch}'. VsDevCmd.bat did not complete within 180 seconds."
            )
            break
        except subprocess.CalledProcessError as exc:
            last_error = exc
            stdout = (exc.stdout or "").strip()
            stderr = (exc.stderr or "").strip()
            if stdout:
                logger.error(f"VsDevCmd stdout ({host_arch}->{target_arch}):\n{stdout}")
            if stderr:
                logger.error(f"VsDevCmd stderr ({host_arch}->{target_arch}):\n{stderr}")
            continue

    if result is None:
        if isinstance(last_error, RuntimeError):
            raise last_error
        raise RuntimeError(
            f"Failed to import MSVC environment for architecture '{target_arch}' via VsDevCmd.bat."
        )

    imported = 0
    for line in result.stdout.splitlines():
        if "=" not in line:
            continue
        key, value = line.split("=", 1)
        os.environ[key] = value
        imported += 1

    if not os.environ.get("VCINSTALLDIR"):
        raise RuntimeError("MSVC environment import failed: VCINSTALLDIR is not set after VsDevCmd.")

    logger.info(f"Imported MSVC environment for {target_arch} ({imported} variables updated).")
