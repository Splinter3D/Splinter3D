#pragma once
#include <fstream>
#include <string>

namespace wx::config
{

    // ─────────────────────────────────────────────
    //  AppConfig — typed settings + file persistence
    // ─────────────────────────────────────────────
    struct AppConfig
    {
        std::string lang     = "default";
        bool        dark     = false;
        int         fontSize = 11;

        static AppConfig load(const std::string& path = "./splinter3D.cfg")
        {
            AppConfig     cfg;
            std::ifstream f(path);
            if (!f.good())
                return cfg;
            std::string key, val;
            while (f >> key >> val)
            {
                if (key == "lang")
                    cfg.lang = val;
                if (key == "dark")
                    cfg.dark = (val == "1");
                if (key == "fontSize")
                    cfg.fontSize = std::stoi(val);
            }
            return cfg;
        }

        void save(const std::string& path = "./splinter3D.cfg") const
        {
            std::ofstream f(path, std::ios::trunc);
            f << "lang " << lang << "\n";
            f << "dark " << (dark ? 1 : 0) << "\n";
            f << "fontSize " << fontSize << "\n";
        }
    };

} // namespace wx