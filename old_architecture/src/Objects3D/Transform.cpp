#include <Objects3D/Transform.hpp>

namespace objects3D
{

    Matrix Transform::toMatrix() const
    {
        return MatrixScale(scale.x, scale.y, scale.z) *
               MatrixRotateXYZ(rotation.toRadian().toRaylib()) *
               MatrixTranslate(position.x, position.y, position.z);
    }
} // namespace objects3D
