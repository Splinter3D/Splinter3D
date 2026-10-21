#pragma once

#include <filesystem>
#include <string>

namespace geometry::occt
{
    /**
     * @brief Converts a path to the UTF-8 string OpenCascade expects for file
     * names on every platform.
     *
     * path.string() would give the ANSI code page on Windows, which breaks
     * any path with non-ASCII characters there.
     */
    [[nodiscard]] inline std::string toUtf8(const std::filesystem::path& path)
    {
        const std::u8string utf8 = path.u8string();
        return std::string(utf8.begin(), utf8.end());
    }
} // namespace geometry::occt
