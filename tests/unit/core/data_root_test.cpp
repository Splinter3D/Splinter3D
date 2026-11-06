#include "core/utils/data_root.hpp"
#include "support/temp_dir.hpp"

#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

using tests::support::TempCwd;

TEST_CASE("findDataRoot: returns an empty path when no candidate has data", "[core][data_root]")
{
    TempCwd tmp;
    REQUIRE(core::utils::findDataRoot({}).empty());
}

TEST_CASE("findDataRoot: prefers the current working directory", "[core][data_root]")
{
    TempCwd tmp;
    fs::create_directory(tmp.path() / "locale");

    REQUIRE(fs::equivalent(core::utils::findDataRoot({}), tmp.path()));
}

TEST_CASE("findDataRoot: falls back to the executable's directory", "[core][data_root]")
{
    TempCwd        tmp;
    const fs::path exeDir = tmp.path() / "install" / "bin";
    fs::create_directories(exeDir / "assets");

    REQUIRE(core::utils::findDataRoot(exeDir / "splinter3D") == exeDir);
}

TEST_CASE("findDataRoot: falls back to the executable's parent directory", "[core][data_root]")
{
    TempCwd        tmp;
    const fs::path installDir = tmp.path() / "install";
    fs::create_directories(installDir / "bin");
    fs::create_directories(installDir / "locale");

    REQUIRE(core::utils::findDataRoot(installDir / "bin" / "splinter3D") == installDir);
}

TEST_CASE("findDataRoot: finds Resources inside a macOS app bundle", "[core][data_root]")
{
    TempCwd        tmp;
    const fs::path contents = tmp.path() / "Splinter3D.app" / "Contents";
    fs::create_directories(contents / "MacOS");
    fs::create_directories(contents / "Resources" / "assets");

    REQUIRE(core::utils::findDataRoot(contents / "MacOS" / "splinter3D") == contents / "Resources");
}

TEST_CASE("findAssetFile: returns an empty path when the asset is missing", "[core][data_root]")
{
    TempCwd tmp;
    REQUIRE(core::utils::findAssetFile(tmp.path() / "bin" / "splinter3D", "icons/logo.png").empty());
}

TEST_CASE("findAssetFile: finds assets next to bin/ in a release archive", "[core][data_root]")
{
    TempCwd        tmp;
    const fs::path root = tmp.path() / "release";
    fs::create_directories(root / "bin");
    fs::create_directories(root / "assets" / "icons");
    std::ofstream(root / "assets" / "icons" / "logo.png") << "x";

    REQUIRE(core::utils::findAssetFile(root / "bin" / "splinter3D.exe", "icons/logo.png") ==
            root / "assets" / "icons" / "logo.png");
}

TEST_CASE("findAssetFile: finds assets in a macOS app bundle's Resources", "[core][data_root]")
{
    TempCwd        tmp;
    const fs::path contents = tmp.path() / "Splinter3D.app" / "Contents";
    fs::create_directories(contents / "MacOS");
    fs::create_directories(contents / "Resources" / "assets" / "icons");
    std::ofstream(contents / "Resources" / "assets" / "icons" / "logo.png") << "x";

    REQUIRE(core::utils::findAssetFile(contents / "MacOS" / "splinter3D", "icons/logo.png") ==
            contents / "Resources" / "assets" / "icons" / "logo.png");
}
