"""Runs the test suites (--tests: unit, integration, then end-to-end) and, optionally, generates a code coverage report
(--coverage). Everything they produce goes to TEST_OUTPUT_DIR (build/test-output/)."""

import os
import sys
import pathlib
import shutil
import subprocess

from Logger import logger
from ParseArgs import args
from Platform import Platform, get_platform

__all__ = ["TEST_OUTPUT_DIR", "generate_coverage", "run_tests", "test_executable_name"]

# Under build/ so it is git-ignored and removed by --clean/--fclean like the rest of the build.
TEST_OUTPUT_DIR = pathlib.Path("build") / "test-output"
_PROFRAW_PATTERN = "tests-*.profraw"
_PROFDATA = TEST_OUTPUT_DIR / "tests.profdata"
_COVERAGE_REPORT = TEST_OUTPUT_DIR / "code_coverage.txt"

# Suites run by --tests, in order (see cmake/8-Tests.cmake). A suite whose executable
# was not built (no sources yet) is skipped. Integration tests open real wx windows.
_SUITES = (
    ("unit", "splinter3D_unit_tests", False),
    ("integration", "splinter3D_integration_tests", True),
)
_APP_TARGET = "splinter3D"
_E2E_DIR = pathlib.Path("tests") / "end_to_end"


def test_executable_name(target: str = "splinter3D_unit_tests") -> str:
    return f"{target}.exe" if get_platform() == Platform.WINDOWS else target


def _resolve_test_executable(target: str) -> pathlib.Path | None:
    # Single-config generators (Ninja/Make) put the executable directly under build/ or
    # build/bin; multi-config generators (Visual Studio) nest it under a per-configuration
    # subdirectory instead, hence checking every configuration name here.
    executable_name = test_executable_name(target)
    build_root = pathlib.Path("build")
    candidates = [
        build_root / "bin" / executable_name,
        build_root / executable_name,
        build_root / "Debug" / executable_name,
        build_root / "Release" / executable_name,
        build_root / "RelWithDebInfo" / executable_name,
        build_root / "MinSizeRel" / executable_name,
    ]
    return next((candidate for candidate in candidates if candidate.exists()), None)


def _built_suites() -> list[tuple[str, pathlib.Path, bool]]:
    suites = []
    for label, target, needs_display in _SUITES:
        executable = _resolve_test_executable(target)
        if executable is None:
            logger.info(f"No {label} test executable built; skipping {label} tests.")
            continue
        suites.append((label, executable, needs_display))
    return suites


def _display_wrapper(needs_display: bool) -> list[str]:
    """On a headless Linux machine (CI, Docker), GUI suites run inside a virtual X server."""
    if not needs_display or get_platform() != Platform.LINUX or os.environ.get("DISPLAY"):
        return []
    if shutil.which("xvfb-run") is None:
        raise RuntimeError("No DISPLAY and xvfb-run is not installed: install xvfb to run the integration tests.")
    return ["xvfb-run", "--auto-servernum", "--server-args=-screen 0 1920x1080x24"]


def run_tests():
    suites = _built_suites()
    if not suites:
        raise RuntimeError("No test executable found: build with --tests first.")
    if args.dry_run:
        for _, executable, _ in suites:
            logger.info(f"DRY RUN: {executable}")
        return
    TEST_OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    # Raw profiles from earlier runs belong to older binaries; merging them into this
    # run's report would skew it (or fail), so start from a clean slate.
    for stale_profile in TEST_OUTPUT_DIR.glob(_PROFRAW_PATTERN):
        stale_profile.unlink()

    env = {
        **os.environ,
        # One raw profile per process, named the way generate_coverage() globs them.
        "LLVM_PROFILE_FILE": str((TEST_OUTPUT_DIR / "tests-%p.profraw").resolve()),
        # Mesa's software renderer: lets OpenGL (the OCCT viewer) work under Xvfb.
        "LIBGL_ALWAYS_SOFTWARE": os.environ.get("LIBGL_ALWAYS_SOFTWARE", "1"),
    }
    failed = []
    for label, executable, needs_display in suites:
        logger.info(f"Running {label} tests (output in {TEST_OUTPUT_DIR})...")
        command = [*_display_wrapper(needs_display), str(executable.resolve())]
        # Run from the output folder so any file a test writes relative to the cwd lands there too.
        result = subprocess.run(command, env=env, cwd=TEST_OUTPUT_DIR)
        if result.returncode != 0:
            logger.error(f"{label} tests failed (exit code {result.returncode}).")
            failed.append(label)
    if not _run_end_to_end_tests(env):
        failed.append("end-to-end")
    if failed:
        raise RuntimeError(f"Tests failed: {', '.join(failed)}")


def _run_end_to_end_tests(env: dict[str, str]) -> bool:
    """Runs tests/end_to_end (Python unittest) against the freshly built app binary."""
    app = _resolve_test_executable(_APP_TARGET)
    if app is None:
        logger.info("No splinter3D binary built; skipping end-to-end tests.")
        return True
    logger.info("Running end-to-end tests...")
    # The GUI smoke test needs a display, hence the same wrapper as integration tests;
    # batch-mode tests strip DISPLAY themselves to prove they run headless.
    command = [*_display_wrapper(True), sys.executable, "-m", "unittest", "discover", "-s", str(_E2E_DIR.resolve()), "-v"]
    result = subprocess.run(command, env={**env, "SPLINTER3D_BIN": str(app.resolve())}, cwd=TEST_OUTPUT_DIR)
    if result.returncode != 0:
        logger.error(f"end-to-end tests failed (exit code {result.returncode}).")
        return False
    return True


def generate_coverage():
    """Writes a coverage report covering every suite to build/test-output/code_coverage.txt:
    llvm-cov/xcrun on macOS (matching its Clang-based toolchain), gcovr elsewhere when
    available, or a placeholder note otherwise."""
    logger.info("Generating coverage report...")
    if args.dry_run:
        logger.info("DRY RUN: generate coverage report")
        return
    TEST_OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    with open(_COVERAGE_REPORT, "w", encoding="utf-8") as coverage_file:
        if get_platform() == Platform.DARWIN:
            executables = [str(executable) for _, executable, _ in _built_suites()]
            # The app binary ran during the end-to-end tests: include its coverage too.
            app = _resolve_test_executable(_APP_TARGET)
            if app is not None:
                executables.append(str(app))
            subprocess.run(
                ["xcrun", "llvm-profdata", "merge", "-sparse", *map(str, TEST_OUTPUT_DIR.glob(_PROFRAW_PATTERN)), "-o", str(_PROFDATA)],
                check=True,
            )
            # llvm-cov takes the first binary positionally and any further ones via -object.
            objects = [executables[0], *(f"-object={executable}" for executable in executables[1:])]
            subprocess.run(
                [
                    "xcrun",
                    "llvm-cov",
                    "report",
                    *objects,
                    f"-instr-profile={_PROFDATA}",
                    "-ignore-filename-regex=.*/tests/.*",
                    "-enable-name-compression",
                ],
                check=True,
                stdout=coverage_file,
            )
            logger.info(f"Coverage report written to {_COVERAGE_REPORT}")
            return
        if shutil.which("gcovr"):
            # GCC writes its .gcda/.gcno data next to the object files (under build/
            # already); only the report itself needs redirecting.
            subprocess.run(["gcovr", "-r", ".", "--exclude", "tests/"], check=True, stdout=coverage_file)
            logger.info(f"Coverage report written to {_COVERAGE_REPORT}")
            return
        coverage_file.write("gcovr not installed; coverage skipped.\n")
