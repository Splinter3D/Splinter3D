#include "preference_manager.hpp"

#include <charconv>
#include <fstream>
#include <system_error>

namespace platform::preference
{
    namespace
    {
        // Parses the whole of value as an int; leaves out untouched on any error
        // (not a number, trailing characters, out of range) so a hand-edited or
        // corrupt file falls back to the default instead of throwing.
        void parseInt(const std::string& value, int& out)
        {
            int         parsed   = 0;
            const auto* end      = value.data() + value.size();
            const auto [ptr, ec] = std::from_chars(value.data(), end, parsed);
            if (ec == std::errc() && ptr == end)
                out = parsed;
        }
    } // namespace

    Preference PreferenceManager::load(const std::string& path)
    {
        Preference Preference;

        std::ifstream file(path);

        if (!file.good())
            return Preference;

        // One "key value" pair per line. The value is the rest of the line, so it
        // may contain spaces; a trailing '\r' (file saved with Windows line
        // endings) is not part of it. Lines without a value are ignored.
        std::string line;
        while (std::getline(file, line))
        {
            if (!line.empty() && line.back() == '\r')
                line.pop_back();

            const auto separator = line.find(' ');
            if (separator == std::string::npos)
                continue;

            const std::string key   = line.substr(0, separator);
            const std::string value = line.substr(separator + 1);

            if (key == "language")
                Preference.language = value;

            else if (key == "darkMode")
                Preference.darkMode = (value == "1");

            else if (key == "fontSize")
                parseInt(value, Preference.fontSize);
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

} // namespace platform::preference
