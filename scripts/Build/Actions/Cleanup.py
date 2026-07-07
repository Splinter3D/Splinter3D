import glob
import os
import pathlib
import shutil

from Logger import logger
from ParseArgs import args

__all__ = ["clean", "fclean"]

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
)


def clean():
    if args.dry_run:
        logger.info("DRY RUN: remove build")
        return
    logger.info("Cleaning build directory...")
    shutil.rmtree("build", ignore_errors=True)


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
