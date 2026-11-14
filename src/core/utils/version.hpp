#pragma once

#include <string_view>

namespace core::utils
{
#if !defined(SPLINTER3D_VERSION)
#error "SPLINTER3D_VERSION is set by CMake from vcpkg.json (cmake/7-Target.cmake)"
#endif

    /// The application version, e.g. "0.2.1" (vcpkg.json's "version-semver").
    inline constexpr std::string_view kVersion = SPLINTER3D_VERSION;

    /// True in a release build (build.py --release, what CI ships), false in a dev build.
#if defined(SPLINTER3D_RELEASE_BUILD)
    inline constexpr bool kReleaseBuild = true;
#else
    inline constexpr bool kReleaseBuild = false;
#endif
} // namespace core::utils
