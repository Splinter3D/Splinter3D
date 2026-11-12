#pragma once

#include <string_view>

namespace core::utils
{
#if !defined(SPLINTER3D_VERSION)
#error "SPLINTER3D_VERSION is set by CMake from vcpkg.json (cmake/7-Target.cmake)"
#endif

    /// The application version, e.g. "0.2.1" (vcpkg.json's "version-semver").
    inline constexpr std::string_view kVersion = SPLINTER3D_VERSION;
} // namespace core::utils
