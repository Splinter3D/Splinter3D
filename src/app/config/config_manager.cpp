#include "config_manager.hpp"

#include <fstream>

namespace app::config
{

    Config ConfigManager::load(const std::string &path)
    {
        Config config;

        std::ifstream file(path);

        if (!file.good())
            return config;

        std::string key;
        std::string value;

        while (file >> key >> value)
        {
            if (key == "language")
                config.language = value;

            else if (key == "darkMode")
                config.darkMode = (value == "1");

            else if (key == "fontSize")
                config.fontSize = std::stoi(value);
        }

        return config;
    }

    void ConfigManager::save(const Config &config, const std::string &path)
    {
        std::ofstream file(path, std::ios::trunc);

        file << "language "
             << config.language
             << "\n";

        file << "darkMode "
             << (config.darkMode ? 1 : 0)
             << "\n";

        file << "fontSize "
             << config.fontSize
             << "\n";
    }

} // namespace app::config
