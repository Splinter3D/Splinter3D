#pragma once

#include <gp_Trsf.hxx>
#include <gp_XYZ.hxx>

namespace geometry::occt
{
    /**
     * @brief Absolute placement of a model: a move (millimeters) and a
     * rotation (degrees) around each world axis.
     *
     * The rotation is applied around the model's own origin, X first, then Y,
     * then Z, and the model is then moved by (moveX, moveY, moveZ) - so the
     * model's pivot always ends up exactly at the move position.
     */
    struct Transform
    {
        double moveX   = 0.0;
        double moveY   = 0.0;
        double moveZ   = 0.0;
        double rotateX = 0.0;
        double rotateY = 0.0;
        double rotateZ = 0.0;

        friend bool operator==(const Transform&, const Transform&) = default;
    };

    /**
     * @brief Builds the OpenCascade transformation for a Transform
     * (translation * rotationZ * rotationY * rotationX).
     */
    [[nodiscard]] gp_Trsf toTrsf(const Transform& transform);

    /**
     * @brief Returns start moved by a translation delta (millimeters).
     */
    [[nodiscard]] Transform withTranslationDelta(Transform start, const gp_XYZ& delta);

    /**
     * @brief Returns start rotated by the rotation part of delta around one
     * world axis.
     *
     * @param axisIndex 0 for X, 1 for Y, anything else for Z.
     * @param delta A transformation whose rotation is around that axis. Its
     * angle is counted positive when its axis points the same way as the world
     * axis, negative otherwise. A delta without rotation leaves start unchanged.
     */
    [[nodiscard]] Transform withRotationDelta(Transform start, int axisIndex, const gp_Trsf& delta);
} // namespace geometry::occt
