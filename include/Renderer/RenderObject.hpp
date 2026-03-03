#pragma once

#include <Objects3D/Object3D.hpp>
#include <Objects3D/ObjectObserver.hpp>
#include <raylib.h>
#include <raymath.h>

namespace renderer
{
    struct RenderObject : objects3D::ObjectObserver
    {
        const objects3D::Object3D* object = nullptr;
        Matrix                     modelMatrix;

        RenderObject() = default;

        void bind(objects3D::Object3D& obj)
        {
            object = &obj;
            obj.attach(this);
            rebuildMatrix();
        }

        void onTransformChanged() override
        {
            rebuildMatrix();
        }

        void onAppearanceChanged() override
        {
            // No appearance data to update in this implementation
        }

        void rebuildMatrix()
        {
            modelMatrix =
                MatrixScale(object->transform.scale.x, object->transform.scale.y, object->transform.scale.z) *
                MatrixRotateXYZ(object->transform.rotation.toRaylib()) *
                MatrixTranslate(object->transform.position.x, object->transform.position.y, object->transform.position.z);
        }
    };
} // namespace renderer