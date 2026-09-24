import os
import sys

from Logger import logger
from ParseArgs import args
from Actions import build_existing_target, clean, fclean, generate_coverage, run_tests
from ConfigureBuild import configure_build


def sanitize_macos_build_environment():
    if sys.platform != "darwin":
        return

    # Apple Clang searches paths from these variables before vcpkg's include
    # and library paths. Homebrew headers can then be mixed with vcpkg
    # packages (for example pnglibconf.h from Homebrew with zlib from vcpkg).
    for variable in ("CPATH", "C_INCLUDE_PATH", "CPLUS_INCLUDE_PATH", "LIBRARY_PATH"):
        os.environ.pop(variable, None)


def main():
    sanitize_macos_build_environment()

    if args.debug:
        logger.set_debug(True)

    logger.info("Starting build process...")

    if args.clean or args.fclean or args.rebuild or args.reinstall_dependencies:
        clean()

    if args.fclean or args.rebuild:
        fclean()

    if args.clean and not args.rebuild and not args.tests:
        logger.info("Build process completed successfully.")
        return

    if args.fclean and not args.rebuild and not args.tests:
        logger.info("Build process completed successfully.")
        return

    if args.skip_configure:
        build_existing_target("splinter3D_unit_tests" if args.tests else "splinter3D")
        if args.tests and not args.skip_build:
            run_tests()
            if args.coverage:
                generate_coverage()
    elif args.tests:
        configure_build(enable_tests=True)
        if not args.skip_build:
            run_tests()
            if args.coverage:
                generate_coverage()
    else:
        configure_build()

    logger.info("Build process completed successfully.")

if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        logger.error(str(e))
        exit(1)
