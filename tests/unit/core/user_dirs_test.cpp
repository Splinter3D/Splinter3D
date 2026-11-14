#include "core/utils/user_dirs.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstdlib>
#include <optional>
#include <string>

namespace fs = std::filesystem;

namespace
{
    // Sets an environment variable for one scope, then restores it.
    class ScopedEnvironment
    {
      public:
        ScopedEnvironment(const char* name, const char* value)
            : name_(name)
        {
            if (const char* previous = std::getenv(name))
                previous_ = previous;
            set(value);
        }

        ~ScopedEnvironment()
        {
            set(previous_ ? previous_->c_str() : nullptr);
        }

        ScopedEnvironment(const ScopedEnvironment&)            = delete;
        ScopedEnvironment& operator=(const ScopedEnvironment&) = delete;

      private:
        void set(const char* value)
        {
#if defined(_WIN32)
            _putenv_s(name_, value != nullptr ? value : "");
#else
            if (value != nullptr)
                ::setenv(name_, value, 1);
            else
                ::unsetenv(name_);
#endif
        }

        const char*                name_;
        std::optional<std::string> previous_;
    };
} // namespace

#if defined(_WIN32)
TEST_CASE("userDataDir / userLocalDataDir: roaming and local AppData", "[core][user_dirs]")
{
    ScopedEnvironment roaming("APPDATA", "C:\\Users\\me\\AppData\\Roaming");
    ScopedEnvironment local("LOCALAPPDATA", "C:\\Users\\me\\AppData\\Local");
    CHECK(core::utils::userDataDir() == fs::path("C:\\Users\\me\\AppData\\Roaming") / "Splinter3D");
    CHECK(core::utils::userLocalDataDir() == fs::path("C:\\Users\\me\\AppData\\Local") / "Splinter3D");
}
#elif defined(__APPLE__)
TEST_CASE("userDataDir / userLocalDataDir: Application Support", "[core][user_dirs]")
{
    ScopedEnvironment home("HOME", "/Users/me");
    const fs::path    expected = fs::path("/Users/me") / "Library" / "Application Support" / "Splinter3D";
    CHECK(core::utils::userDataDir() == expected);
    CHECK(core::utils::userLocalDataDir() == expected);
}
#else
TEST_CASE("userDataDir / userLocalDataDir: XDG folders", "[core][user_dirs]")
{
    ScopedEnvironment home("HOME", "/home/me");
    {
        ScopedEnvironment config("XDG_CONFIG_HOME", nullptr);
        ScopedEnvironment data("XDG_DATA_HOME", nullptr);
        CHECK(core::utils::userDataDir() == fs::path("/home/me/.config/Splinter3D"));
        CHECK(core::utils::userLocalDataDir() == fs::path("/home/me/.local/share/Splinter3D"));
    }
    ScopedEnvironment config("XDG_CONFIG_HOME", "/xdg/config");
    ScopedEnvironment data("XDG_DATA_HOME", "/xdg/data");
    CHECK(core::utils::userDataDir() == fs::path("/xdg/config/Splinter3D"));
    CHECK(core::utils::userLocalDataDir() == fs::path("/xdg/data/Splinter3D"));
}
#endif

#if !defined(_WIN32)
TEST_CASE("userDataDir / userLocalDataDir: empty without a home", "[core][user_dirs]")
{
    ScopedEnvironment home("HOME", nullptr);
    ScopedEnvironment config("XDG_CONFIG_HOME", nullptr);
    ScopedEnvironment data("XDG_DATA_HOME", nullptr);
    CHECK(core::utils::userDataDir().empty());
    CHECK(core::utils::userLocalDataDir().empty());
}
#endif

TEST_CASE("settingsDir / profilesDir: dev builds keep their own under the data root", "[core][user_dirs]")
{
    const fs::path dataRoot("/opt/splinter3D");
    if constexpr (core::utils::kReleaseBuild)
    {
        CHECK(core::utils::settingsDir(dataRoot) == core::utils::userDataDir());
        CHECK(core::utils::profilesDir(dataRoot) == core::utils::userLocalDataDir());
    }
    else
    {
        CHECK(core::utils::settingsDir(dataRoot) == dataRoot / "dev-data");
        CHECK(core::utils::profilesDir(dataRoot) == dataRoot / "dev-data");
    }
}
