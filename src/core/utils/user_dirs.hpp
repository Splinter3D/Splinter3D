#pragma once

#include <cstdlib>
#include <filesystem>
#include <string>

namespace core::utils
{
    /// The folder name used under each per-user location.
    inline constexpr const char* kUserDirName = "Splinter3D";

    /** @brief An environment variable as a path (read wide on Windows, so non-ASCII names survive); empty if unset. */
    inline std::filesystem::path environmentPath(const char* name)
    {
#if defined(_WIN32)
        const std::wstring wide(name, name + std::char_traits<char>::length(name));
        wchar_t*           value = nullptr;
        std::size_t        size  = 0;
        if (_wdupenv_s(&value, &size, wide.c_str()) != 0 || value == nullptr)
            return {};
        std::filesystem::path path(value);
        std::free(value);
        return path;
#else
        const char* value = std::getenv(name);
        return value != nullptr && *value != '\0' ? std::filesystem::path(value) : std::filesystem::path();
#endif
    }

    namespace detail
    {
        inline std::filesystem::path under(const std::filesystem::path& base)
        {
            return base.empty() ? base : base / kUserDirName;
        }
    } // namespace detail

    /**
     * @brief Where the user's own files go (settings): per user, writable,
     * outside the install. May follow the user to other machines.
     *
     * %APPDATA%\\Splinter3D on Windows, ~/Library/Application Support/Splinter3D
     * on macOS, $XDG_CONFIG_HOME/Splinter3D (default ~/.config/Splinter3D)
     * elsewhere. Empty if the environment doesn't say where.
     */
    inline std::filesystem::path userDataDir()
    {
#if defined(_WIN32)
        return detail::under(environmentPath("APPDATA"));
#elif defined(__APPLE__)
        const auto home = environmentPath("HOME");
        return home.empty() ? home : detail::under(home / "Library" / "Application Support");
#else
        if (const auto config = environmentPath("XDG_CONFIG_HOME"); !config.empty())
            return detail::under(config);
        const auto home = environmentPath("HOME");
        return home.empty() ? home : detail::under(home / ".config");
#endif
    }

    /**
     * @brief Where data about this machine goes (learned scalability
     * profiles): per user, writable, never roaming.
     *
     * %LOCALAPPDATA%\\Splinter3D on Windows, ~/Library/Application
     * Support/Splinter3D on macOS, $XDG_DATA_HOME/Splinter3D (default
     * ~/.local/share/Splinter3D) elsewhere. Empty if the environment doesn't
     * say where.
     */
    inline std::filesystem::path userLocalDataDir()
    {
#if defined(_WIN32)
        return detail::under(environmentPath("LOCALAPPDATA"));
#elif defined(__APPLE__)
        return userDataDir();
#else
        if (const auto data = environmentPath("XDG_DATA_HOME"); !data.empty())
            return detail::under(data);
        const auto home = environmentPath("HOME");
        return home.empty() ? home : detail::under(home / ".local" / "share");
#endif
    }
} // namespace core::utils
