"""clean()/fclean() implement --clean/--fclean/--rebuild: removing the build/ directory
and, for a full clean, build artifacts left in the repository root."""

import glob
import os
import pathlib
import shutil

from Logger import logger
from ParseArgs import args

__all__ = ["clean", "fclean"]

# Artifacts fclean() removes from the repository root (as opposed to clean(), which only
# clears build/): compiled binaries/libraries and leftover coverage/profiling output.
_FCLEAN_ARTIFACTS = (
    "*.so",
    "*.dylib",
    "*.dll",
    "*.lib",
    "*.exp",
    "*.a",
    "*.ilk",
    "*.pdb",
    "splinter3D_unit_tests",
    "splinter3D_unit_tests.exe",
    "splinter3D",
    "splinter3D.exe",
    "unit_tests",
    "plugins",
    "code_coverage.txt",
    "unit_tests-*.profraw",
    "unit_tests.profdata",
    "vgcore*",
    "cmake-build-debug",
    "build"
)


def clean():
    # Keep build/bin and build/lib: only CMake's cache/config state is cleared, not the
    # binaries already built into them.
    ignore_patterns = ["bin", "lib"]
    build_glob = glob.glob("build/*", recursive=True) + glob.glob("build/.*", recursive=True)
    build_glob = [path for path in build_glob if not any(pattern in path for pattern in ignore_patterns)]

    if args.dry_run:
        logger.info("DRY RUN: remove build")
        return
    logger.info("Cleaning build directory...")
    for path in build_glob:
        if os.path.isdir(path) and not os.path.islink(path):
            shutil.rmtree(path, ignore_errors=True)
        else:
            pathlib.Path(path).unlink(missing_ok=True)


def fclean():
    if args.dry_run:
        logger.info("DRY RUN: remove build artifacts")
        return
    logger.info("Removing build artifacts...")
    for pattern in _FCLEAN_ARTIFACTS:
        for path in glob.glob(pattern):
            if os.path.isdir(path) and not os.path.islink(path):
                shutil.rmtree(path, ignore_errors=True)
            else:
                pathlib.Path(path).unlink(missing_ok=True)
