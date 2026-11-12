#include "application/service/scene_service.hpp"
#include "support/progress_probe.hpp"
#include "support/temp_dir.hpp"

#include <BRep_Tool.hxx>
#include <Poly_Triangulation.hxx>
#include <TopExp_Explorer.hxx>
#include <TopLoc_Location.hxx>
#include <TopoDS.hxx>
#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
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
        return shape.triangleCount(); // imported models are meshes: count their triangles
    }

    // Exact bounds from the triangulation nodes, placed by each face's location.
    std::pair<gp_Pnt, gp_Pnt> boundingBox(const Shape& shape)
    {
        constexpr double kHuge = 1e100;
        gp_Pnt           min(kHuge, kHuge, kHuge);
        gp_Pnt           max(-kHuge, -kHuge, -kHuge);
        for (TopExp_Explorer it(shape.value(), TopAbs_FACE); it.More(); it.Next())
        {
            TopLoc_Location location;
            const auto      mesh = BRep_Tool::Triangulation(TopoDS::Face(it.Current()), location);
            if (mesh.IsNull())
                continue;
            for (int i = 1; i <= mesh->NbNodes(); ++i)
            {
                const gp_Pnt p = mesh->Node(i).Transformed(location.Transformation());
                min.SetCoord(std::min(min.X(), p.X()), std::min(min.Y(), p.Y()), std::min(min.Z(), p.Z()));
                max.SetCoord(std::max(max.X(), p.X()), std::max(max.Y(), p.Y()), std::max(max.Z(), p.Z()));
            }
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

TEST_CASE("SceneService: load times the import", "[application][scene]")
{
    SceneService scene;
    scene.load(kCube);
    scene.add("built", Shape{});

    CHECK(scene.models()[0].loadDuration > platform::benchmark::Duration::zero());
    CHECK(scene.models()[1].loadDuration == platform::benchmark::Duration::zero());
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

TEST_CASE("SceneService: remove one model", "[application][scene]")
{
    SceneService scene;
    scene.add("a", Shape{});
    scene.add("b", Shape{});
    scene.add("c", Shape{});
    scene.setTransform(2, kSomeTransform);

    REQUIRE(scene.remove(1));

    CHECK(scene.names() == std::vector<std::string>{"a", "c"});
    // Later models shift down, keeping their transform.
    CHECK(scene.transform(1) == kSomeTransform);

    CHECK_FALSE(scene.remove(2));
    CHECK_FALSE(scene.remove(-1));
    CHECK_FALSE(scene.remove(SceneService::kAllModels));
    CHECK(scene.size() == 2);
}

TEST_CASE("SceneService: ids are unique and never reused", "[application][scene]")
{
    SceneService scene;
    scene.add("a", Shape::fromSTL(kCube));
    scene.add("b", Shape::fromSTL(kCube));
    const auto first  = scene.models()[0].id;
    const auto second = scene.models()[1].id;
    REQUIRE(scene.remove(0));
    scene.add("c", Shape::fromSTL(kCube));
    const auto third = scene.models()[1].id;

    CHECK(first != 0);
    CHECK(first < second);
    CHECK(second < third);
}

TEST_CASE("SceneService: find and indexOf follow removals", "[application][scene]")
{
    SceneService scene;
    scene.add("a", Shape::fromSTL(kCube));
    scene.add("b", Shape::fromSTL(kCube));
    const auto removed = scene.models()[0].id;
    const auto kept    = scene.models()[1].id;
    REQUIRE(scene.remove(0));

    CHECK(scene.find(removed) == nullptr);
    CHECK_FALSE(scene.indexOf(removed));
    REQUIRE(scene.find(kept) != nullptr);
    CHECK(scene.find(kept)->name == "b");
    CHECK(scene.indexOf(kept) == 0u);
    CHECK(scene.ids() == std::vector<application::service::ModelId>{kept});
}

TEST_CASE("SceneService: add with a reserved id keeps it", "[application][scene]")
{
    SceneService scene;
    const auto   reserved = scene.reserveId();
    scene.add("other", Shape::fromSTL(kCube));
    scene.add("reserved", Shape::fromSTL(kCube), reserved);

    CHECK(scene.find(reserved)->name == "reserved");
    CHECK(scene.models()[0].id != reserved);
}

TEST_CASE("SceneService: removeById and setTransformById", "[application][scene]")
{
    SceneService scene;
    scene.add("a", Shape::fromSTL(kCube));
    const auto id = scene.models()[0].id;

    REQUIRE(scene.setTransformById(id, kSomeTransform));
    CHECK(scene.transform(0).moveZ == 3);
    CHECK_FALSE(scene.setTransformById(id + 100, kSomeTransform));
    CHECK_FALSE(scene.removeById(id + 100));
    REQUIRE(scene.removeById(id));
    CHECK(scene.empty());
}

TEST_CASE("exportSTL: writes through a temporary file", "[application][scene]")
{
    const tests::support::TempDir dir;
    const auto                    output = dir.path() / "out.stl";
    SceneService                  scene;
    scene.add("cube", Shape::fromSTL(kCube));

    occ::handle<tests::support::ProgressProbe> probe = new tests::support::ProgressProbe();
    REQUIRE(application::service::exportSTL(scene.models(), output, probe->Start()));
    CHECK(std::filesystem::file_size(output) > 0);
    CHECK_FALSE(tests::support::hasTemporaryFiles(dir.path()));
    CHECK(probe->maxPosition >= 0.99);
}

TEST_CASE("exportSTL: leaves nothing behind when cancelled", "[application][scene]")
{
    const tests::support::TempDir dir;
    const auto                    output = dir.path() / "out.stl";
    std::ofstream(output) << "previous";
    SceneService scene;
    scene.add("cube", Shape::fromSTL(kCube));

    occ::handle<tests::support::ProgressProbe> probe = new tests::support::ProgressProbe();
    probe->cancel                                    = true;
    CHECK_FALSE(application::service::exportSTL(scene.models(), output, probe->Start()));
    CHECK_FALSE(tests::support::hasTemporaryFiles(dir.path()));
    std::ifstream     previous(output);
    std::stringstream content;
    content << previous.rdbuf();
    CHECK(content.str() == "previous");
}

TEST_CASE("exportSTL: refuses an empty model list", "[application][scene]")
{
    const tests::support::TempDir dir;
    CHECK_FALSE(application::service::exportSTL({}, dir.path() / "out.stl"));
    CHECK_FALSE(std::filesystem::exists(dir.path() / "out.stl"));
}

TEST_CASE("exportSTL: concurrent exports to one path both produce a whole file", "[application][scene]")
{
    const tests::support::TempDir dir;
    const auto                    output   = dir.path() / "out.stl";
    const auto                    magnolia = std::filesystem::path(SPLINTER3D_TEST_ASSETS_DIR) / "stl" / "binary" / "magnolia_binary.stl";
    SceneService                  scene;
    scene.add("magnolia", Shape::fromSTL(magnolia));
    const int expectedFaces = countFaces(scene.models()[0].shape);

    for (int round = 0; round < 3; ++round)
    {
        bool        first  = false;
        bool        second = false;
        std::thread other([&] { first = application::service::exportSTL(scene.models(), output); });
        second = application::service::exportSTL(scene.models(), output);
        other.join();
        CHECK(first);
        CHECK(second);
        CHECK(countFaces(Shape::fromSTL(output)) == expectedFaces);
    }
    for (const auto& entry : std::filesystem::directory_iterator(dir.path()))
        CHECK(entry.path().filename() == "out.stl"); // no temporary left behind
}

TEST_CASE("exportSTL: leaves the scene's shapes untouched", "[application][scene]")
{
    const tests::support::TempDir dir;
    SceneService                  scene;
    scene.add("cube", Shape::fromSTL(kCube));

    std::vector<const Poly_Triangulation*> before;
    for (TopExp_Explorer it(scene.models()[0].shape.value(), TopAbs_FACE); it.More(); it.Next())
    {
        TopLoc_Location location;
        before.push_back(BRep_Tool::Triangulation(TopoDS::Face(it.Current()), location).get());
    }
    REQUIRE(application::service::exportSTL(scene.models(), dir.path() / "out.stl"));

    std::size_t i = 0;
    for (TopExp_Explorer it(scene.models()[0].shape.value(), TopAbs_FACE); it.More(); it.Next(), ++i)
    {
        TopLoc_Location location;
        CHECK(BRep_Tool::Triangulation(TopoDS::Face(it.Current()), location).get() == before[i]);
    }
}

TEST_CASE("exportSTL: writes mesh models as they are", "[application][scene]")
{
    const tests::support::TempDir dir;
    const auto                    output = dir.path() / "out.stl";
    const auto                    source = std::filesystem::path(SPLINTER3D_TEST_ASSETS_DIR) / "stl" / "binary" / "model.stl";
    SceneService                  scene;
    scene.add("model", Shape::fromSTL(source));

    REQUIRE(application::service::exportSTL(scene.models(), output));
    const auto original = Shape::fromSTL(source);
    const auto reloaded = Shape::fromSTL(output);
    CHECK(reloaded.triangleCount() == original.triangleCount());
    const auto [min, max]   = boundingBox(reloaded);
    const auto [omin, omax] = boundingBox(original);
    CHECK(min.Distance(omin) < 1e-6);
    CHECK(max.Distance(omax) < 1e-6);
}

TEST_CASE("exportSTL: mixes mesh and B-Rep models", "[application][scene]")
{
    const tests::support::TempDir dir;
    const auto                    output = dir.path() / "out.stl";
    SceneService                  scene;
    scene.add("mesh", Shape::fromSTL(kCube));
    scene.add("brep", Shape::toBRep(Shape::fromSTL(kCube))); // e.g. a future cut's result
    scene.setTransform(1, Transform{.moveX = 10});

    REQUIRE(application::service::exportSTL(scene.models(), output));
    const auto reloaded = Shape::fromSTL(output);
    CHECK(reloaded.triangleCount() == 24);
    CHECK_THAT(boundingBox(reloaded).second.X(), WithinAbs(11, 1e-4));
}

TEST_CASE("SceneService: setBRep and brep", "[application][scene]")
{
    SceneService scene;
    scene.add("cube", Shape::fromSTL(kCube));
    const auto id = scene.models()[0].id;
    CHECK(scene.brep(id) == nullptr);

    REQUIRE(scene.setBRep(id, Shape::toBRep(scene.models()[0].shape)));
    REQUIRE(scene.brep(id) != nullptr);
    CHECK_FALSE(scene.brep(id)->isNull());
    CHECK_FALSE(scene.models()[0].shape.mesh().IsNull()); // still displayed and exported as a mesh
    CHECK_FALSE(scene.setBRep(id + 100, Shape::toBRep(scene.models()[0].shape)));
}
