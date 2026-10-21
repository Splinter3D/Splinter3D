#include "application/service/scene_service.hpp"
#include "application/workflow/batch_runner.hpp"
#include "support/temp_dir.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

using application::service::SceneService;
using application::workflow::BatchRunner;
using Catch::Matchers::ContainsSubstring;
using Catch::Matchers::StartsWith;
using geometry::occt::Transform;

namespace
{
    const std::filesystem::path kStlDir = std::filesystem::path(SPLINTER3D_TEST_ASSETS_DIR) / "stl";

    class BatchFixture
    {
      public:
        BatchRunner::Result run(const std::string& json)
        {
            return runner.run(json, kStlDir);
        }

        SceneService       scene;
        std::ostringstream log;
        BatchRunner        runner{scene, log};
    };
} // namespace

TEST_CASE_METHOD(BatchFixture, "BatchRunner: an empty script succeeds", "[application][batch]")
{
    const auto result = run(R"({"steps": []})");

    CHECK(result.ok);
    CHECK(result.stepsRun == 0);
}

TEST_CASE_METHOD(BatchFixture, "BatchRunner: open resolves paths against the base directory", "[application][batch]")
{
    const auto result = run(R"({"steps": [{"open": "ascii/cube.stl"}, {"open": "binary/cube.stl"}]})");

    REQUIRE(result.ok);
    CHECK(result.stepsRun == 2);
    CHECK(scene.names() == std::vector<std::string>{"cube.stl", "cube.stl (2)"});
    CHECK_THAT(log.str(), ContainsSubstring("step 2/2: open") && ContainsSubstring("model 1 'cube.stl (2)'"));
}

TEST_CASE_METHOD(BatchFixture, "BatchRunner: transform targets one model or all", "[application][batch]")
{
    const auto result = run(R"({"steps": [
        {"open": "ascii/cube.stl"},
        {"open": "ascii/cube.stl"},
        {"transform": {"move": [1, 2, 3]}},
        {"transform": {"target": 1, "move": [10, 0, 0], "rotate": [0, 0, 90]}}
    ]})");

    REQUIRE(result.ok);
    CHECK(scene.transform(0) == Transform{.moveX = 1, .moveY = 2, .moveZ = 3});
    CHECK(scene.transform(1) == Transform{.moveX = 10, .rotateZ = 90});
}

TEST_CASE_METHOD(BatchFixture, "BatchRunner: export and clear", "[application][batch]")
{
    tests::support::TempDir tmp;
    const auto              output = tmp.path() / "out.stl";

    const auto result = runner.run(R"({"steps": [
        {"open": ")" + (kStlDir / "ascii" / "cube.stl").generic_string() +
                                       R"("},
        {"export": "out.stl"},
        {"clear": true}
    ]})",
                                   tmp.path());

    REQUIRE(result.ok);
    CHECK(std::filesystem::file_size(output) > 0);
    CHECK(scene.empty());
}

TEST_CASE_METHOD(BatchFixture, "BatchRunner: stops at the first failing step", "[application][batch]")
{
    const auto result = run(R"({"steps": [
        {"open": "ascii/cube.stl"},
        {"open": "missing.stl"},
        {"clear": true}
    ]})");

    CHECK_FALSE(result.ok);
    CHECK(result.stepsRun == 1);
    CHECK_THAT(result.error, StartsWith("step 2/3 (open): "));
    CHECK(scene.size() == 1); // "clear" never ran
}

TEST_CASE_METHOD(BatchFixture, "BatchRunner: reports invalid steps", "[application][batch]")
{
    struct Case
    {
        const char* script;
        const char* error;
    };

    const Case cases[] = {
        {R"({"steps": [{"explode": true}]})", "unknown action \"explode\""},
        {R"({"steps": [{"open": "a.stl", "clear": true}]})", "exactly one action"},
        {R"({"steps": [{"open": 3}]})", "expected a non-empty path string"},
        {R"({"steps": [{"transform": {"move": [1, 2]}}]})", "\"move\" must be an array of 3 numbers"},
        {R"({"steps": [{"transform": {"target": "first"}}]})", "\"target\" must be a model index or \"all\""},
        {R"({"steps": [{"transform": {"target": 0}}]})", "no such target (0 model(s) loaded)"},
        {R"({"steps": [{"export": "out.stl"}]})", "(no model loaded)"},
    };

    for (const auto& [script, error] : cases)
    {
        CAPTURE(script);
        const auto result = run(script);
        CHECK_FALSE(result.ok);
        CHECK_THAT(result.error, ContainsSubstring(error));
    }
}

TEST_CASE_METHOD(BatchFixture, "BatchRunner: rejects malformed scripts", "[application][batch]")
{
    CHECK_THAT(run("not json").error, StartsWith("invalid JSON"));
    CHECK_THAT(run("[]").error, ContainsSubstring("\"steps\" array"));
    CHECK_THAT(run(R"({"steps": {}})").error, ContainsSubstring("\"steps\" array"));
}

TEST_CASE_METHOD(BatchFixture, "BatchRunner: runFile resolves paths against the script's folder", "[application][batch]")
{
    tests::support::TempDir tmp;
    std::filesystem::copy_file(kStlDir / "ascii" / "cube.stl", tmp.path() / "model.stl");
    std::ofstream(tmp.path() / "script.json") << R"({"steps": [{"open": "model.stl"}]})";

    CHECK(runner.runFile(tmp.path() / "script.json").ok);
    CHECK(scene.names() == std::vector<std::string>{"model.stl"});

    CHECK_THAT(runner.runFile(tmp.path() / "missing.json").error, StartsWith("cannot read script"));
}
