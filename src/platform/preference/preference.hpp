#pragma once

#include <string>

namespace plateform::preference
{
    /**
     * @brief Stores persistent application settings.
     *
     * This structure only contains user preferences.
     * File loading and saving are handled by ConfigManager.
     */
    struct Preference
    {
        std::string language = "default";
        bool        darkMode = false;
        int         fontSize = 11;
    };

} // namespace plateform::preference
