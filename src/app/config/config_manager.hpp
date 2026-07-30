#pragma once

#include "config.hpp"

#include <string>

namespace app::config
{

    /**
     * @brief Handles loading and saving application configuration.
     */
    class ConfigManager
    {
    public:
        /**
         * @brief Loads configuration from disk.
         *
         * @param path Configuration file path.
         *
         * @return Loaded configuration.
         */
        static Config load(const std::string &path = "./splinter3D.cfg");

        /**
         * @brief Saves configuration to disk.
         *
         * @param config Configuration to save.
         *
         * @param path Configuration file path.
         */
        static void save(const Config &config, const std::string &path = "./splinter3D.cfg");
    };

} // namespace app::config
