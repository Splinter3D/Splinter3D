#include "geometry/occt/shape.hpp"

#include <TopAbs_ShapeEnum.hxx>
#include <TopExp_Explorer.hxx>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
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
    // A cube is 6 quads, i.e. 12 triangular facets.
    REQUIRE(countFaces(shape.value()) == 12);
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
