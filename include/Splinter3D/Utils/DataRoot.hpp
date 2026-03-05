/*
** EPITECH PROJECT, 2026
** Prototype
** File description:
** Data root discovery helper
*/

#pragma once

#include <filesystem>
#include <vector>

namespace splinter::utils
{

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

} // namespace splinter::utils
