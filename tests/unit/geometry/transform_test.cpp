#include "geometry/occt/transform.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <gp.hxx>
#include <gp_Ax1.hxx>
#include <gp_Pnt.hxx>
#include <numbers>

using Catch::Matchers::WithinAbs;
using geometry::occt::Transform;

namespace
{
    constexpr double kEps = 1e-9;

    void requirePoint(const gp_Pnt& actual, double x, double y, double z)
    {
        CHECK_THAT(actual.X(), WithinAbs(x, kEps));
        CHECK_THAT(actual.Y(), WithinAbs(y, kEps));
        CHECK_THAT(actual.Z(), WithinAbs(z, kEps));
    }

    gp_Pnt apply(const Transform& transform, gp_Pnt point)
    {
        point.Transform(geometry::occt::toTrsf(transform));
        return point;
    }

    gp_Trsf rotation(const gp_Dir& axis, double degrees)
    {
        gp_Trsf trsf;
        trsf.SetRotation(gp_Ax1(gp::Origin(), axis), degrees * std::numbers::pi / 180.0);
        return trsf;
    }
} // namespace

TEST_CASE("toTrsf: identity transform leaves points in place", "[geometry][transform]")
{
    requirePoint(apply({}, gp_Pnt(1, 2, 3)), 1, 2, 3);
}

TEST_CASE("toTrsf: move translates by millimeters", "[geometry][transform]")
{
    requirePoint(apply({.moveX = 10, .moveY = -5, .moveZ = 2.5}, gp_Pnt(1, 1, 1)), 11, -4, 3.5);
}

TEST_CASE("toTrsf: rotation angles are in degrees around world axes", "[geometry][transform]")
{
    requirePoint(apply({.rotateZ = 90}, gp_Pnt(1, 0, 0)), 0, 1, 0);
    requirePoint(apply({.rotateX = 90}, gp_Pnt(0, 1, 0)), 0, 0, 1);
    requirePoint(apply({.rotateY = 90}, gp_Pnt(0, 0, 1)), 1, 0, 0);
}

TEST_CASE("toTrsf: rotates X first, then Z, around the origin, then moves", "[geometry][transform]")
{
    // X first: (0,1,0) -> (0,0,1), which Z leaves alone. Z first would give (-1,0,0).
    requirePoint(apply({.rotateX = 90, .rotateZ = 90}, gp_Pnt(0, 1, 0)), 0, 0, 1);

    // The pivot is the model origin, so it always lands exactly on the move.
    requirePoint(apply({.moveX = 5, .moveY = 6, .moveZ = 7, .rotateX = 30, .rotateY = 45, .rotateZ = 60},
                       gp::Origin()),
                 5, 6, 7);
}

TEST_CASE("withTranslationDelta: adds to the starting move only", "[geometry][transform]")
{
    const Transform start{.moveX = 1, .moveY = 2, .moveZ = 3, .rotateZ = 15};

    const Transform result = geometry::occt::withTranslationDelta(start, gp_XYZ(10, 0, -1));

    CHECK(result == Transform{.moveX = 11, .moveY = 2, .moveZ = 2, .rotateZ = 15});
}

TEST_CASE("withRotationDelta: adds the signed angle to the dragged axis", "[geometry][transform]")
{
    const Transform start{.moveX = 4, .rotateX = 10, .rotateY = 20, .rotateZ = 30};

    SECTION("positive rotation around the matching axis")
    {
        const Transform result = geometry::occt::withRotationDelta(start, 2, rotation(gp::DZ(), 45));
        CHECK_THAT(result.rotateZ, WithinAbs(75, kEps));
        CHECK(result.rotateX == start.rotateX);
        CHECK(result.rotateY == start.rotateY);
        CHECK(result.moveX == start.moveX);
    }

    SECTION("rotation around the opposite axis counts as negative")
    {
        const Transform result = geometry::occt::withRotationDelta(start, 0, rotation(-gp::DX(), 25));
        CHECK_THAT(result.rotateX, WithinAbs(-15, kEps));
    }

    SECTION("axis index 1 targets Y")
    {
        const Transform result = geometry::occt::withRotationDelta(start, 1, rotation(gp::DY(), 5));
        CHECK_THAT(result.rotateY, WithinAbs(25, kEps));
    }

    SECTION("a delta without rotation changes nothing")
    {
        gp_Trsf translationOnly;
        translationOnly.SetTranslation(gp_Vec(1, 2, 3));
        CHECK(geometry::occt::withRotationDelta(start, 2, translationOnly) == start);
    }
}

namespace
{
    void requireSameMapping(const gp_Trsf& actual, const gp_Trsf& expected)
    {
        for (const gp_Pnt& point : {gp_Pnt(0, 0, 0), gp_Pnt(1, 0, 0), gp_Pnt(0, 1, 0), gp_Pnt(0, 0, 1), gp_Pnt(3, -2, 5)})
        {
            const gp_Pnt want = point.Transformed(expected);
            requirePoint(point.Transformed(actual), want.X(), want.Y(), want.Z());
        }
    }

    gp_Trsf translation(double x, double y, double z)
    {
        gp_Trsf trsf;
        trsf.SetTranslation(gp_Vec(x, y, z));
        return trsf;
    }
} // namespace

TEST_CASE("fromTrsf: inverts toTrsf", "[geometry][transform]")
{
    for (const Transform& transform : {Transform{10, 20, 30, 15, -40, 70}, Transform{0, 0, 0, 0, 90, 30}, Transform{}})
    {
        const gp_Trsf trsf = geometry::occt::toTrsf(transform);
        requireSameMapping(geometry::occt::toTrsf(geometry::occt::fromTrsf(trsf)), trsf);
    }
}

TEST_CASE("placeInGroup: rotates around the pivot, then moves", "[geometry][transform]")
{
    using geometry::occt::placeInGroup;

    requirePoint(apply(placeInGroup({}, gp_Pnt(10, 0, 0), {0, 0, 0, 0, 0, 90}), gp_Pnt(0, 0, 0)), 10, -10, 0);
    requirePoint(apply(placeInGroup({}, gp_Pnt(10, 0, 0), {5, 0, 0, 0, 0, 90}), gp_Pnt(0, 0, 0)), 15, -10, 0);

    const Transform start{1, 2, 3, 0, 0, 45};
    const gp_Pnt    pivot(4, 4, 0);
    const Transform offset{7, 0, 0, 30, 0, 0};
    const gp_Trsf   expected = translation(7, 0, 0) * translation(4, 4, 0) * geometry::occt::toTrsf({0, 0, 0, 30, 0, 0}) *
                               translation(-4, -4, 0) * geometry::occt::toTrsf(start);
    requireSameMapping(geometry::occt::toTrsf(placeInGroup(start, pivot, offset)), expected);

    requireSameMapping(geometry::occt::toTrsf(placeInGroup(start, pivot, {})), geometry::occt::toTrsf(start));
}
