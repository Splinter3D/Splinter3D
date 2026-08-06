#pragma once

/**
 * @file api.hpp
 * @brief Cross-platform symbol visibility macros.
 *
 * These macros control which symbols are exported from shared libraries.
 * On Windows, dllexport/dllimport is handled by CMake's
 * WINDOWS_EXPORT_ALL_SYMBOLS, so SPLINTER3D_API is currently a no-op
 * there. On GCC/Clang, it maps to default visibility.
 */

#if defined(_WIN32)
/**
 * @brief Marks a symbol as part of the public API of a shared library.
 */
#define SPLINTER3D_API
#else
#define SPLINTER3D_API __attribute__((visibility("default")))
#endif

#if defined(__GNUC__) || defined(__clang__)
/**
 * @brief Marks a symbol as hidden from the shared library's public ABI.
 */
#define SPLINTER3D_HIDE_FROM_ABI __attribute__((visibility("hidden")))
#else
#define SPLINTER3D_HIDE_FROM_ABI
#endif
