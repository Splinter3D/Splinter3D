#pragma once

#include <string>

namespace platform::preference
{
    /**
     * @brief Stores persistent application settings.
     *
     * This structure only contains user preferences.
     * File loading and saving are handled by PreferenceManager.
     */
    struct Preference
    {
        std::string language = "default";
        bool        darkMode = false;
        int         fontSize = 11;
    };

} // namespace platform::preference
