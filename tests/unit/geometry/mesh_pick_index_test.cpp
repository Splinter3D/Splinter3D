#include "geometry/occt/mesh_pick_index.hpp"
#include "geometry/occt/shape.hpp"

#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <Poly_Triangulation.hxx>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>
#include <filesystem>
#include <gp_Dir.hxx>
#include <gp_Lin.hxx>
#include <gp_Pnt.hxx>
#include <optional>

using Catch::Matchers::WithinAbs;
using geometry::occt::MeshPickIndex;
using geometry::occt::Shape;

namespace
{
    const std::filesystem::path kCube = std::filesystem::path(SPLINTER3D_TEST_ASSETS_DIR) / "stl" / "ascii" / "cube.stl";

    gp_Lin down(double x, double y)
    {
        return gp_Lin(gp_Pnt(x, y, 10), gp_Dir(0, 0, -1));
    }

    // A bumpy n x n grid over [0, n]^2, z = height(x, y) at each node.
    double height(double x, double y)
    {
        return std::sin(x * 0.7) + std::cos(y * 0.3) * 2.0;
    }

    Shape terrain(int n)
    {
        occ::handle<Poly_Triangulation> mesh = new Poly_Triangulation((n + 1) * (n + 1), 2 * n * n, false);
        for (int j = 0; j <= n; ++j)
            for (int i = 0; i <= n; ++i)
                mesh->SetNode(j * (n + 1) + i + 1, gp_Pnt(i, j, height(i, j)));
        int t = 1;
        for (int j = 0; j < n; ++j)
            for (int i = 0; i < n; ++i)
            {
                const int a = j * (n + 1) + i + 1;
                const int b = a + 1;
                const int c = a + n + 1;
                const int d = c + 1;
                mesh->SetTriangle(t++, Poly_Triangle(a, b, d));
                mesh->SetTriangle(t++, Poly_Triangle(a, d, c));
            }
        return Shape::fromMesh(mesh);
    }
} // namespace

TEST_CASE("MeshPickIndex: a ray down onto a cube hits its top face", "[geometry][pick]")
{
    const auto index = MeshPickIndex::build(Shape::fromSTL(kCube));
    REQUIRE(index != nullptr);
    const auto hit = index->intersect(down(0.5, 0.5));
    REQUIRE(hit.has_value());
    CHECK_THAT(*hit, WithinAbs(9.0, 1e-9)); // from z=10 to the top at z=1
}

TEST_CASE("MeshPickIndex: a ray beside the model misses", "[geometry][pick]")
{
    const auto index = MeshPickIndex::build(Shape::fromSTL(kCube));
    REQUIRE(index != nullptr);
    CHECK_FALSE(index->intersect(down(5, 5)).has_value());
    CHECK_FALSE(index->intersect(down(1.5, 0.5)).has_value());
}

TEST_CASE("MeshPickIndex: the first hit is the nearest to where the ray comes from", "[geometry][pick]")
{
    const auto index = MeshPickIndex::build(Shape::fromSTL(kCube));
    REQUIRE(index != nullptr);
    // The line's origin is past the cube: hits behind it count, the bottom one first.
    const auto hit = index->intersect(gp_Lin(gp_Pnt(0.5, 0.5, 10), gp_Dir(0, 0, 1)));
    REQUIRE(hit.has_value());
    CHECK_THAT(*hit, WithinAbs(-10.0, 1e-9));
}

TEST_CASE("MeshPickIndex: B-Rep faces are picked through their triangulation", "[geometry][pick]")
{
    BRepPrimAPI_MakeBox box(gp_Pnt(2, 0, 0), 1, 1, 1);
    const Shape         shape(box.Shape());
    CHECK(MeshPickIndex::build(shape) == nullptr); // nothing meshed yet

    BRepMesh_IncrementalMesh mesher(shape.value(), 0.1);
    const auto               index = MeshPickIndex::build(shape);
    REQUIRE(index != nullptr);
    const auto hit = index->intersect(down(2.5, 0.5));
    REQUIRE(hit.has_value());
    CHECK_THAT(*hit, WithinAbs(9.0, 1e-9));
}

TEST_CASE("MeshPickIndex: agrees with the surface on a large mesh", "[geometry][pick]")
{
    constexpr int n     = 120; // 28 800 triangles: deep enough to exercise the tree
    const auto    index = MeshPickIndex::build(terrain(n));
    REQUIRE(index != nullptr);

    for (int j = 0; j < n; j += 7)
        for (int i = 0; i < n; i += 5)
        {
            // Exactly over a grid node, the surface height is known.
            const auto hit = index->intersect(down(i, j));
            CAPTURE(i, j);
            REQUIRE(hit.has_value());
            CHECK_THAT(*hit, WithinAbs(10.0 - height(i, j), 1e-9));
        }
    CHECK_FALSE(index->intersect(down(n + 1.0, 3)).has_value());
}
