#pragma once

#include "preference.hpp"

#include <string>

namespace plateform::preference
{

    /**
     * @brief Handles loading and saving application configuration.
     */
    class PreferenceManager
    {
      public:
        /**
         * @brief Loads configuration from disk.
         *
         * @param path Configuration file path.
         *
         * @return Loaded configuration.
         */
        static Preference load(const std::string& path = "./splinter3D.cfg");

        /**
         * @brief Saves configuration to disk.
         *
         * @param preference Configuration to save.
         *
         * @param path Configuration file path.
         */
        static void save(const Preference& preference, const std::string& path = "./splinter3D.cfg");
    };

} // namespace plateform::preference
