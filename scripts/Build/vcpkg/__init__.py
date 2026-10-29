# Re-exports the vcpkg lifecycle helpers: detect an existing install, install/bootstrap one,
# and compute the CMake toolchain/triplet flags for it.
from .Detect import detect_vcpkg
from .Install import install_vcpkg
from .SetTargets import set_vcpkg_targets

__all__ = ["detect_vcpkg", "install_vcpkg", "set_vcpkg_targets"]
