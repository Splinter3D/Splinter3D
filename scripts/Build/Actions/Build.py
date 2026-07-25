import os
import pathlib

from ParseArgs import args
from ConfigureBuild.Cache import ensure_compatible_build_cache
from ConfigureBuild.BuildSystem import run_build_tool, _detect_build_system_from_cache, BuildSystem
from ConfigureBuild.MSVC import ensure_msvc_environment
from Platform import Platform, get_platform

__all__ = ["build_existing_target"]


def build_existing_target(target: str):
    build_dir = pathlib.Path("build")
    ensure_compatible_build_cache(build_dir)
    if not build_dir.is_dir():
        raise RuntimeError("Build directory does not exist. Cannot use --skip-configure.")
    cwd = os.getcwd()
    try:
        os.chdir(build_dir)
        build_system = _detect_build_system_from_cache(pathlib.Path.cwd())
        if get_platform() == Platform.WINDOWS and build_system == BuildSystem.NINJA:
            ensure_msvc_environment()
        build_type = "Debug" if args.debug_build or args.debug or target == "splinter3D_unit_tests" else "Release"
        run_build_tool(None, target, build_type)
    finally:
        os.chdir(cwd)
