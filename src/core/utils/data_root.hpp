#pragma once

#include <filesystem>
#include <vector>

namespace core::utils
{

    /**
     * @brief Locates the application's data root directory.
     *
     * Searches in order the current working directory, the
     * executable's directory, the executable's parent directory, and
     * the sibling "Resources" directory (macOS .app bundle layout:
     * Contents/MacOS/<exe> next to Contents/Resources), returning the
     * first one that contains either a "locale" or an "assets" subfolder.
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
            {
                candidates.push_back(exePath.parent_path().parent_path());
                candidates.push_back(exePath.parent_path().parent_path() / "Resources");
            }
        }
        for (const auto& c : candidates)
        {
            if (std::filesystem::exists(c / "locale") || std::filesystem::exists(c / "assets"))
                return c;
        }
        return std::filesystem::path();
    }

    /**
     * @brief Locates a file under an "assets" folder near the executable.
     *
     * Checks "<root>/assets/<relative>" for each root, in order: the
     * current working directory, the executable's directory and its
     * ancestors up to four levels (build trees such as build/bin/Debug,
     * or bin/ inside a release archive), and the sibling "Resources"
     * directory (macOS .app bundle: Contents/MacOS next to
     * Contents/Resources).
     *
     * @param exePath Absolute path to the running executable. May be
     * empty, in which case only the current working directory is checked.
     * @param relative Path of the file relative to the assets folder,
     * e.g. "icons/logo_borderless.png".
     *
     * @return The first existing candidate, or an empty path if none exist.
     */
    inline std::filesystem::path findAssetFile(const std::filesystem::path& exePath,
                                               const std::filesystem::path& relative)
    {
        std::vector<std::filesystem::path> roots;
        roots.push_back(std::filesystem::current_path());
        if (!exePath.empty())
        {
            auto dir = exePath.parent_path();
            if (dir.has_parent_path())
                roots.push_back(dir.parent_path() / "Resources");
            for (int depth = 0; depth < 4 && !dir.empty(); ++depth)
            {
                roots.push_back(dir);
                if (dir == dir.parent_path())
                    break;
                dir = dir.parent_path();
            }
        }
        for (const auto& root : roots)
        {
            const auto candidate = root / "assets" / relative;
            if (std::filesystem::exists(candidate))
                return candidate;
        }
        return std::filesystem::path();
    }

} // namespace core::utils
