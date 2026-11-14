#include "transform.hpp"

#include <gp.hxx>
#include <gp_Ax1.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <gp_Quaternion.hxx>
#include <gp_Vec.hxx>
#include <numbers>

namespace geometry::occt
{
    namespace
    {
        constexpr double kDegToRad = std::numbers::pi / 180.0;
        constexpr double kRadToDeg = 180.0 / std::numbers::pi;

        gp_Trsf rotationAround(const gp_Dir& axis, double degrees)
        {
            gp_Trsf rotation;
            rotation.SetRotation(gp_Ax1(gp::Origin(), axis), degrees * kDegToRad);
            return rotation;
        }
    } // namespace

    gp_Trsf toTrsf(const Transform& transform)
    {
        gp_Trsf translation;
        translation.SetTranslation(gp_Vec(transform.moveX, transform.moveY, transform.moveZ));

        return translation * rotationAround(gp::DZ(), transform.rotateZ) * rotationAround(gp::DY(), transform.rotateY) *
               rotationAround(gp::DX(), transform.rotateX);
    }

    Transform fromTrsf(const gp_Trsf& trsf)
    {
        double x = 0.0;
        double y = 0.0;
        double z = 0.0;
        trsf.GetRotation().GetEulerAngles(gp_Extrinsic_XYZ, x, y, z);
        const gp_XYZ& move = trsf.TranslationPart();
        return {move.X(), move.Y(), move.Z(), x * kRadToDeg, y * kRadToDeg, z * kRadToDeg};
    }

    Transform placeInGroup(const Transform& start, const gp_Pnt& pivot, const Transform& offset)
    {
        gp_Trsf toPivot;
        toPivot.SetTranslation(gp_Vec(pivot.XYZ()));
        const Transform rotation{0.0, 0.0, 0.0, offset.rotateX, offset.rotateY, offset.rotateZ};
        gp_Trsf         move;
        move.SetTranslation(gp_Vec(offset.moveX, offset.moveY, offset.moveZ));
        return fromTrsf(move * toPivot * toTrsf(rotation) * toPivot.Inverted() * toTrsf(start));
    }

    Transform withTranslationDelta(Transform start, const gp_XYZ& delta)
    {
        start.moveX += delta.X();
        start.moveY += delta.Y();
        start.moveZ += delta.Z();
        return start;
    }

    Transform withRotationDelta(Transform start, int axisIndex, const gp_Trsf& delta)
    {
        gp_XYZ axis;
        double angleRad = 0.0;
        if (!delta.GetRotation(axis, angleRad))
            return start;

        const gp_Dir expected = axisIndex == 0 ? gp::DX() : axisIndex == 1 ? gp::DY()
                                                                           : gp::DZ();
        const double sign     = axis.Dot(expected.XYZ()) >= 0.0 ? 1.0 : -1.0;
        const double deltaDeg = angleRad * kRadToDeg * sign;

        if (axisIndex == 0)
            start.rotateX += deltaDeg;
        else if (axisIndex == 1)
            start.rotateY += deltaDeg;
        else
            start.rotateZ += deltaDeg;
        return start;
    }
} // namespace geometry::occt
