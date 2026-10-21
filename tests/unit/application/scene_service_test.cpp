#include "application/service/scene_service.hpp"
#include "support/temp_dir.hpp"

#include <BRep_Tool.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using application::service::SceneService;
using Catch::Matchers::WithinAbs;
using geometry::occt::Shape;
using geometry::occt::Transform;

namespace
{
    int countFaces(const Shape& shape)
    {
        int count = 0;
        for (TopExp_Explorer it(shape.value(), TopAbs_FACE); it.More(); it.Next())
            ++count;
        return count;
    }

    // Exact bounds from the vertices (an STL shape is only flat triangles).
    std::pair<gp_Pnt, gp_Pnt> boundingBox(const Shape& shape)
    {
        constexpr double kHuge = 1e100;
        gp_Pnt           min(kHuge, kHuge, kHuge);
        gp_Pnt           max(-kHuge, -kHuge, -kHuge);
        for (TopExp_Explorer it(shape.value(), TopAbs_VERTEX); it.More(); it.Next())
        {
            const gp_Pnt p = BRep_Tool::Pnt(TopoDS::Vertex(it.Current()));
            min.SetCoord(std::min(min.X(), p.X()), std::min(min.Y(), p.Y()), std::min(min.Z(), p.Z()));
            max.SetCoord(std::max(max.X(), p.X()), std::max(max.Y(), p.Y()), std::max(max.Z(), p.Z()));
        }
        return {min, max};
    }

    const std::filesystem::path kCube = std::filesystem::path(SPLINTER3D_TEST_ASSETS_DIR) / "stl" / "ascii" / "cube.stl";

    const Transform kSomeTransform{.moveX = 1, .moveY = 2, .moveZ = 3, .rotateX = 10, .rotateY = 20, .rotateZ = 30};
} // namespace

TEST_CASE("SceneService: starts empty", "[application][scene]")
{
    const SceneService scene;

    CHECK(scene.empty());
    CHECK(scene.size() == 0);
    CHECK(scene.names().empty());
}

TEST_CASE("SceneService: load names models after their file", "[application][scene]")
{
    SceneService scene;

    REQUIRE(scene.load(kCube) == 0);

    CHECK_FALSE(scene.empty());
    CHECK(scene.names() == std::vector<std::string>{"cube.stl"});
    CHECK_FALSE(scene.models().front().shape.isNull());
    CHECK(scene.transform(0) == Transform{});
}

TEST_CASE("SceneService: duplicate names get a numeric suffix", "[application][scene]")
{
    SceneService scene;
    scene.load(kCube);
    scene.load(kCube);
    scene.add("cube.stl", Shape{});
    scene.add("other.stl", Shape{});

    CHECK(scene.names() == std::vector<std::string>{"cube.stl", "cube.stl (2)", "cube.stl (3)", "other.stl"});
}

TEST_CASE("SceneService: a failed load leaves the scene unchanged", "[application][scene]")
{
    SceneService scene;
    scene.load(kCube);

    REQUIRE_THROWS_AS(scene.load(kCube.parent_path() / "missing.stl"), std::runtime_error);
    CHECK(scene.size() == 1);
}

TEST_CASE("SceneService: setTransform on one model", "[application][scene]")
{
    SceneService scene;
    scene.add("a", Shape{});
    scene.add("b", Shape{});

    REQUIRE(scene.setTransform(1, kSomeTransform));

    CHECK(scene.transform(0) == Transform{});
    CHECK(scene.transform(1) == kSomeTransform);
}

TEST_CASE("SceneService: setTransform on every model", "[application][scene]")
{
    SceneService scene;
    scene.add("a", Shape{});
    scene.add("b", Shape{});
    scene.setTransform(0, Transform{.moveX = 99});

    REQUIRE(scene.setTransform(SceneService::kAllModels, kSomeTransform));

    CHECK(scene.transform(0) == kSomeTransform);
    CHECK(scene.transform(1) == kSomeTransform);
    // "All models" has no single transform of its own.
    CHECK(scene.transform(SceneService::kAllModels) == Transform{});
}

TEST_CASE("SceneService: invalid targets are rejected", "[application][scene]")
{
    SceneService scene;

    CHECK_FALSE(scene.setTransform(SceneService::kAllModels, kSomeTransform));
    CHECK_FALSE(scene.setTransform(0, kSomeTransform));

    scene.add("a", Shape{});
    CHECK_FALSE(scene.setTransform(1, kSomeTransform));
    CHECK_FALSE(scene.setTransform(-2, kSomeTransform));
    CHECK(scene.transform(0) == Transform{});
    CHECK(scene.transform(5) == Transform{});
}

TEST_CASE("SceneService: clear removes every model", "[application][scene]")
{
    SceneService scene;
    scene.add("a", Shape{});
    scene.add("a", Shape{});

    scene.clear();

    CHECK(scene.empty());
    // Names are free again after a clear.
    scene.add("a", Shape{});
    CHECK(scene.names() == std::vector<std::string>{"a"});
}

TEST_CASE("SceneService: saveSTL", "[application][scene]")
{
    tests::support::TempDir tmp;
    const auto              output = tmp.path() / "out.stl";
    SceneService            scene;

    SECTION("refuses an empty scene")
    {
        CHECK_FALSE(scene.saveSTL(output));
        CHECK_FALSE(std::filesystem::exists(output));
    }

    SECTION("writes a file that loads back")
    {
        scene.load(kCube);

        REQUIRE(scene.saveSTL(output));

        SceneService reloaded;
        reloaded.load(output);
        CHECK_FALSE(reloaded.models().front().shape.isNull());
    }

    SECTION("writes every model, each at its transform")
    {
        scene.load(kCube);
        scene.load(kCube);
        scene.setTransform(1, Transform{.moveX = 10, .moveY = 20, .moveZ = 30});

        REQUIRE(scene.saveSTL(output));

        const auto reloaded = Shape::fromSTL(output);
        CHECK(countFaces(reloaded) == 24);
        const auto [min, max] = boundingBox(reloaded);
        CHECK_THAT(min.X(), WithinAbs(0, 1e-4));
        CHECK_THAT(max.X(), WithinAbs(11, 1e-4));
        CHECK_THAT(max.Y(), WithinAbs(21, 1e-4));
        CHECK_THAT(max.Z(), WithinAbs(31, 1e-4));
    }

    SECTION("applies rotations")
    {
        scene.load(kCube);
        scene.setTransform(0, Transform{.rotateZ = 90});

        REQUIRE(scene.saveSTL(output));

        // The unit cube turned 90 degrees around Z spans x in [-1, 0].
        const auto [min, max] = boundingBox(Shape::fromSTL(output));
        CHECK_THAT(min.X(), WithinAbs(-1, 1e-4));
        CHECK_THAT(max.X(), WithinAbs(0, 1e-4));
    }
}
