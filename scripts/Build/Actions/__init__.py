from .Build import build_existing_target
from .Cleanup import clean, fclean
from .Tests import generate_coverage, run_tests, test_executable_name

__all__ = [
    "build_existing_target",
    "clean",
    "fclean",
    "generate_coverage",
    "run_tests",
    "test_executable_name",
]
