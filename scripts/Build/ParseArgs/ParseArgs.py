import argparse

__all__ = ["args"]

parser = argparse.ArgumentParser(description="Build script for Splinter3D")
parser.add_argument("--debug", action="store_true", help="Enable debug mode for the build script logs and keep legacy debug-build behavior")
parser.add_argument("-d", "--debug-build", action="store_true", help="Configure and build with debug symbols")
parser.add_argument("--generator", type=str, help="Specify CMake generator (for example: Ninja, Unix Makefiles, Visual Studio 17 2022, Visual Studio 18 2026)")
parser.add_argument("--vcpkg-path", type=str, help="Specify a vcpkg root or a directory in which to install vcpkg")
parser.add_argument("-a", "--auto-mode", "-ci", "--ci", action="store_true", help="Enable auto mode")
parser.add_argument("--skip-submodules", action="store_true", help="Ignore submodules during the build process")
parser.add_argument("-r", "--rebuild", action="store_true", help="Force rebuild of the project")
parser.add_argument("-c", "--clean", action="store_true", help="Clean the build directory before building")
parser.add_argument("-f", "--fclean", action="store_true", help="Clean the build directory and remove the executable")
parser.add_argument("--skip-vcpkg", action="store_true", help="Skip vcpkg installation and configuration")
parser.add_argument("--skip-build", action="store_true", help="Skip the build step after configuration")
parser.add_argument("--skip-configure", action="store_true", help="Skip the configuration step before building")
parser.add_argument("--tests", action="store_true", help="Build and run tests")
parser.add_argument("--coverage", action="store_true", help="Enable code coverage analysis")
parser.add_argument("--reinstall-vcpkg", action="store_true", help="Force reinstall of vcpkg")
parser.add_argument("--reinstall-dependencies", action="store_true", help="Force reinstall of all dependencies")
parser.add_argument("--dry-run", action="store_true", help="Perform a dry run without making any changes")

args = parser.parse_args()
