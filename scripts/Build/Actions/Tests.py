import glob
import pathlib
import shutil
import subprocess

from Logger import logger
from ParseArgs import args
from Platform import Platform, get_platform

__all__ = ["generate_coverage", "run_tests", "test_executable_name"]


def test_executable_name() -> str:
    return "splinter3D_unit_tests.exe" if get_platform() == Platform.WINDOWS else "splinter3D_unit_tests"


def _resolve_test_executable() -> pathlib.Path:
    executable_name = test_executable_name()
    build_root = pathlib.Path("build")
    candidates = [
        build_root / executable_name,
        build_root / "Debug" / executable_name,
        build_root / "Release" / executable_name,
        build_root / "RelWithDebInfo" / executable_name,
        build_root / "MinSizeRel" / executable_name,
    ]
    for candidate in candidates:
        if candidate.exists():
            return candidate
    return build_root / executable_name


def run_tests():
    executable = _resolve_test_executable()
    if args.dry_run:
        logger.info(f"DRY RUN: {executable}")
        return
    logger.info("Running unit tests...")
    try:
        subprocess.run([str(executable)], check=True)
    except subprocess.CalledProcessError as e:
        logger.error(f"Unit tests failed: {e}")
        raise RuntimeError(f"Unit tests failed: {e}")


def generate_coverage():
    logger.info("Generating coverage report...")
    if args.dry_run:
        logger.info("DRY RUN: generate coverage report")
        return
    with open("code_coverage.txt", "w", encoding="utf-8") as coverage_file:
        if get_platform() == Platform.DARWIN:
            subprocess.run(
                ["xcrun", "llvm-profdata", "merge", "-sparse", *glob.glob("unit_tests-*.profraw"), "-o", "unit_tests.profdata"],
                check=True,
            )
            subprocess.run(
                [
                    "xcrun",
                    "llvm-cov",
                    "report",
                    f"./build/{test_executable_name()}",
                    "-instr-profile=unit_tests.profdata",
                    "-ignore-filename-regex=.*/tests/.*",
                    "-enable-name-compression",
                ],
                check=True,
                stdout=coverage_file,
            )
            return
        if shutil.which("gcovr"):
            subprocess.run(["gcovr", "-r", ".", "--exclude", "tests/"], check=True, stdout=coverage_file)
            return
        coverage_file.write("gcovr not installed; coverage skipped.\n")
