import os
import pathlib

from ParseArgs import args
from ConfigureBuild.BuildSystem import run_build_tool

__all__ = ["build_existing_target"]


def build_existing_target(target: str):
    build_dir = pathlib.Path("build")
    if not build_dir.is_dir():
        raise RuntimeError("Build directory does not exist. Cannot use --skip-configure.")
    cwd = os.getcwd()
    try:
        os.chdir(build_dir)
        build_type = "Debug" if args.debug or target == "splinter3D_unit_tests" else "Release"
        run_build_tool(None, target, build_type)
    finally:
        os.chdir(cwd)
