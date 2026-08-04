#pragma once

#include <filesystem>
#include <vector>

namespace core::utils
{

    /**
     * @brief Locates the application's data root directory.
     *
     * Searches in order the current working directory, the
     * executable's directory, and the executable's parent directory,
     * returning the first one that contains either a "locale" or an
     * "assets" subfolder.
     *
     * This allows the application to find its data files regardless of
     * how it was launched (e.g. directly from the build output, or
     * installed under a different layout).
     *
     * @param exePath Absolute path to the running executable, typically
     * obtained via wxStandardPaths in bootstrap.cpp. May be empty, in
     * which case only the current working directory is checked.
     *
     * @return The resolved data root directory, or an empty path if
     * none of the candidates contain "locale" or "assets".
     */
    inline std::filesystem::path findDataRoot(const std::filesystem::path& exePath)
    {
        std::vector<std::filesystem::path> candidates;
        candidates.push_back(std::filesystem::current_path());
        if (!exePath.empty())
        {
            candidates.push_back(exePath.parent_path());
            if (exePath.parent_path().has_parent_path())
                candidates.push_back(exePath.parent_path().parent_path());
        }
        for (const auto& c : candidates)
        {
            if (std::filesystem::exists(c / "locale") || std::filesystem::exists(c / "assets"))
                return c;
        }
        return std::filesystem::path();
    }

} // namespace core::utils
