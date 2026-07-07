import os
import pathlib
import shutil
import subprocess
from Logger import logger
from ParseArgs import args
from vcpkg.SetTargets import set_vcpkg_targets
from Submodules import update_submodules
from Requirements import check_requirements
from Platform import Platform, get_platform
from Platform import Arch, get_arch
from .Cache import ensure_compatible_build_cache
from .BuildSystem import BuildSystem, choose_build_system, BUILD_SYSTEM_TO_STRING, run_build_tool

__all__ = ["configure_build"]

_VISUAL_STUDIO_ARCH = {
    Arch.X86: "Win32",
    Arch.X64: "x64",
    Arch.aarch64: "ARM64",
}

def configure_build(enable_tests: bool = False):
    logger.info("Starting build configuration...")
    check_requirements()
    if args.dry_run:
        logger.info("DRY RUN: skip submodule update")
    else:
        update_submodules()
    vcpkg_targets = [] if args.skip_vcpkg else set_vcpkg_targets()
    cwd = os.getcwd()
    try:
        build_dir = pathlib.Path("build")
        ensure_compatible_build_cache(build_dir)
        if args.dry_run:
            logger.info(f"DRY RUN: create {build_dir}")
        else:
            build_dir.mkdir(exist_ok=True)
        os.chdir("build" if build_dir.exists() else cwd)
        build_system_runtime = choose_build_system()
        build_system_str = BUILD_SYSTEM_TO_STRING[build_system_runtime]
        build_type = "Debug" if args.debug or enable_tests else "Release"
        cmake_command = ["cmake", "..", "-G", build_system_str]
        if build_system_runtime == BuildSystem.NINJA:
            ninja_path = shutil.which("ninja")
            if ninja_path:
                cmake_command.append(f"-DCMAKE_MAKE_PROGRAM={ninja_path}")
        elif build_system_runtime == BuildSystem.MAKE:
            make_path = shutil.which("make")
            if make_path:
                cmake_command.append(f"-DCMAKE_MAKE_PROGRAM={make_path}")
        if build_system_runtime == BuildSystem.VISUAL_STUDIO and get_platform() == Platform.WINDOWS:
            visual_studio_arch = _VISUAL_STUDIO_ARCH.get(get_arch())
            if visual_studio_arch is None:
                raise RuntimeError(f"Unsupported Visual Studio architecture: {get_arch()}")
            cmake_command.extend(["-A", visual_studio_arch])
        if build_system_runtime != BuildSystem.VISUAL_STUDIO:
            cmake_command.append(f"-DCMAKE_BUILD_TYPE={build_type}")
        if args.debug and not enable_tests:
            cmake_command.append("-DENABLE_DEBUG=ON")
        if enable_tests:
            cmake_command.append("-DENABLE_TESTS=ON")
        cmake_command.extend(vcpkg_targets)
        logger.info(f"Configuring CMake with {build_system_str}...")
        if args.dry_run:
            logger.info(f"DRY RUN: {' '.join(cmake_command)}")
        else:
            try:
                subprocess.run(
                    cmake_command,
                    check=True,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                    text=True,
                )
            except subprocess.CalledProcessError as e:
                stdout = (e.stdout or "").strip()
                stderr = (e.stderr or "").strip()
                if stdout:
                    logger.error(f"CMake configure stdout:\n{stdout}")
                if stderr:
                    logger.error(f"CMake configure stderr:\n{stderr}")
                manifest_log = build_dir / "vcpkg-manifest-install.log"
                if manifest_log.is_file():
                    manifest_log_text = manifest_log.read_text(encoding="utf-8", errors="ignore").strip()
                    if manifest_log_text:
                        logger.error(f"vcpkg manifest install log:\n{manifest_log_text}")
                logger.error(f"Build configuration failed: {e}")
                raise RuntimeError(f"Build configuration failed: {e}")
        logger.info("Build configuration completed successfully.")
        if args.skip_build:
            logger.info("Skipping build step as per the provided argument.")
            return
        target = "splinter3D_unit_tests" if enable_tests else "splinter3D"
        logger.info(f"Building target {target} with {build_system_str}...")
        run_build_tool(build_system_runtime, target, build_type)
    finally:
        os.chdir(cwd)
