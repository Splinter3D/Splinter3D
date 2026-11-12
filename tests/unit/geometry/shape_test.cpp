#include "geometry/occt/shape.hpp"
#include "support/progress_probe.hpp"

#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <StlAPI_Reader.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp_Explorer.hxx>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <filesystem>
#include <stdexcept>
#include <string>

namespace
{
    const std::filesystem::path kAssets = SPLINTER3D_TEST_ASSETS_DIR;

    int countFaces(const TopoDS_Shape& shape)
    {
        int count = 0;
        for (TopExp_Explorer it(shape, TopAbs_FACE); it.More(); it.Next())
            ++count;
        return count;
    }
} // namespace

TEST_CASE("Shape: default-constructed shape is null", "[geometry][shape]")
{
    REQUIRE(geometry::occt::Shape{}.isNull());
}

TEST_CASE("Shape::fromSTL: imports the cube fixture", "[geometry][shape]")
{
    const std::string format = GENERATE("ascii", "binary");
    CAPTURE(format);

    const auto shape = geometry::occt::Shape::fromSTL(kAssets / "stl" / format / "cube.stl");

    REQUIRE_FALSE(shape.isNull());
    // A mesh: one face carrying every facet (a cube is 6 quads, i.e. 12 triangles).
    REQUIRE(countFaces(shape.value()) == 1);
    REQUIRE_FALSE(shape.mesh().IsNull());
    CHECK(shape.triangleCount() == 12);
    CHECK_FALSE(shape.mesh()->IsDoublePrecision()); // STL stores floats: nothing lost, half the memory
}

TEST_CASE("Shape::fromSTL: imports in double precision on request", "[geometry][shape]")
{
    const std::string format = GENERATE("ascii", "binary");
    CAPTURE(format);
    using Precision = geometry::occt::Shape::Precision;

    const auto single = geometry::occt::Shape::fromSTL(kAssets / "stl" / format / "cube.stl");
    const auto dbl    = geometry::occt::Shape::fromSTL(kAssets / "stl" / format / "cube.stl", {}, {}, Precision::Double);

    CHECK(single.precision() == Precision::Single);
    CHECK(dbl.precision() == Precision::Double);
    REQUIRE(dbl.mesh()->NbNodes() == single.mesh()->NbNodes());
    for (int i = 1; i <= dbl.mesh()->NbNodes(); ++i)
        CHECK(dbl.mesh()->Node(i).IsEqual(single.mesh()->Node(i), 0.0));
}

TEST_CASE("Shape::withPrecision: converts a mesh both ways, leaves other shapes alone", "[geometry][shape]")
{
    using geometry::occt::Shape;
    const auto single = Shape::fromSTL(kAssets / "stl" / "ascii" / "cube.stl");

    const auto dbl = Shape::withPrecision(single, Shape::Precision::Double);
    CHECK(dbl.precision() == Shape::Precision::Double);
    CHECK(dbl.triangleCount() == single.triangleCount());
    CHECK(Shape::withPrecision(dbl, Shape::Precision::Single).precision() == Shape::Precision::Single);
    CHECK(Shape::withPrecision(single, Shape::Precision::Single).mesh() == single.mesh()); // no copy

    const auto brep = Shape::toBRep(single);
    CHECK_FALSE(brep.precision().has_value());
    CHECK(Shape::withPrecision(brep, Shape::Precision::Double).value().IsSame(brep.value()));
}

TEST_CASE("Shape::fromSTL: rejects an empty path", "[geometry][shape]")
{
    REQUIRE_THROWS_AS(geometry::occt::Shape::fromSTL({}), std::invalid_argument);
}

TEST_CASE("Shape::fromSTL: rejects a missing file", "[geometry][shape]")
{
    REQUIRE_THROWS_AS(geometry::occt::Shape::fromSTL(kAssets / "stl" / "does_not_exist.stl"), std::runtime_error);
}

TEST_CASE("Shape: wraps and exposes an existing TopoDS_Shape", "[geometry][shape]")
{
    auto       imported = geometry::occt::Shape::fromSTL(kAssets / "stl" / "ascii" / "cube.stl");
    const auto wrapped  = geometry::occt::Shape(imported.value());

    REQUIRE_FALSE(wrapped.isNull());
    CHECK(wrapped.value().IsSame(imported.value()));

    imported.value().Nullify(); // the mutable accessor gives write access
    CHECK(imported.isNull());
    CHECK_FALSE(wrapped.isNull());
}

TEST_CASE("Shape::toBRep: builds what StlAPI_Reader builds", "[geometry][shape]")
{
    const std::string file = GENERATE("ascii/cube.stl", "binary/model.stl");
    CAPTURE(file);
    const auto path = kAssets / "stl" / file;

    TopoDS_Shape  expected;
    StlAPI_Reader reader;
    REQUIRE(reader.Read(expected, path.string().c_str()));

    occ::handle<tests::support::ProgressProbe> probe = new tests::support::ProgressProbe();
    const auto                                 brep  = geometry::occt::Shape::toBRep(geometry::occt::Shape::fromSTL(path), probe->Start());

    REQUIRE(countFaces(brep.value()) == countFaces(expected));
    CHECK(brep.mesh().IsNull()); // a face per triangle, no longer a mesh face
    Bnd_Box expectedBox;
    Bnd_Box actualBox;
    BRepBndLib::Add(expected, expectedBox);
    BRepBndLib::Add(brep.value(), actualBox);
    using Catch::Matchers::WithinAbs;
    // Float vertices: equal to within single precision.
    CHECK_THAT(actualBox.CornerMin().Distance(expectedBox.CornerMin()), WithinAbs(0.0, 1e-4));
    CHECK_THAT(actualBox.CornerMax().Distance(expectedBox.CornerMax()), WithinAbs(0.0, 1e-4));
    CHECK(probe->maxPosition >= 0.99);
}

TEST_CASE("Shape::toBRep: stops when cancelled", "[geometry][shape]")
{
    const auto                                 mesh  = geometry::occt::Shape::fromSTL(kAssets / "stl" / "binary" / "model.stl");
    occ::handle<tests::support::ProgressProbe> probe = new tests::support::ProgressProbe();
    probe->cancel                                    = true;
    REQUIRE_THROWS_AS(geometry::occt::Shape::toBRep(mesh, probe->Start()), std::runtime_error);
}

TEST_CASE("Shape::toBRep: refuses a shape that is not a mesh", "[geometry][shape]")
{
    const auto brep = geometry::occt::Shape::toBRep(geometry::occt::Shape::fromSTL(kAssets / "stl" / "ascii" / "cube.stl"));
    REQUIRE_THROWS_AS(geometry::occt::Shape::toBRep(brep), std::invalid_argument);
}

TEST_CASE("Shape::fromSTL: reports progress up to completion", "[geometry][shape]")
{
    occ::handle<tests::support::ProgressProbe> probe = new tests::support::ProgressProbe();
    (void) geometry::occt::Shape::fromSTL(kAssets / "stl" / "binary" / "model.stl", probe->Start());
    CHECK(probe->maxPosition >= 0.99);
}

TEST_CASE("Shape::fromSTL: stops when cancelled", "[geometry][shape]")
{
    occ::handle<tests::support::ProgressProbe> probe = new tests::support::ProgressProbe();
    probe->cancel                                    = true;
    REQUIRE_THROWS_AS(geometry::occt::Shape::fromSTL(kAssets / "stl" / "binary" / "model.stl", probe->Start()),
                      std::runtime_error);
}
