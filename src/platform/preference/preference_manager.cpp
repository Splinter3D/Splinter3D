#include "preference_manager.hpp"

#include <fstream>

namespace plateform::preference
{

    Preference PreferenceManager::load(const std::string& path)
    {
        Preference Preference;

        std::ifstream file(path);

        if (!file.good())
            return Preference;

        std::string key;
        std::string value;

        while (file >> key >> value)
        {
            if (key == "language")
                Preference.language = value;

            else if (key == "darkMode")
                Preference.darkMode = (value == "1");

            else if (key == "fontSize")
                Preference.fontSize = std::stoi(value);
        }

        return Preference;
    }

    void PreferenceManager::save(const Preference& preference, const std::string& path)
    {
        std::ofstream file(path, std::ios::trunc);

        file << "language "
             << preference.language
             << "\n";

        file << "darkMode "
             << (preference.darkMode ? 1 : 0)
             << "\n";

        file << "fontSize "
             << preference.fontSize
             << "\n";
    }

} // namespace plateform::preference
