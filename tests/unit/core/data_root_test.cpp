#include "core/utils/data_root.hpp"
#include "support/temp_dir.hpp"

#include <catch2/catch_test_macros.hpp>
#include <filesystem>

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
