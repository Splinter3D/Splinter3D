#pragma once

#include <string>

namespace app::config
{
    /**
     * @brief Stores persistent application settings.
     *
     * This structure only contains user preferences.
     * File loading and saving are handled by ConfigManager.
     */
    struct Config
    {
        std::string language = "default";
        bool darkMode = false;
        int fontSize = 11;
    };

} // namespace app::config
