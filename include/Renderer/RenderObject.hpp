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

        /**
         * Binds this RenderObject to an Object3D, allowing it to receive transform and appearance updates.
         */
        void bind(objects3D::Object3D& obj)
        {
            object = &obj;
            obj.attach(this);
            rebuildMatrix();
        }

        /**
         * ObjectObserver implementation - called when the observed Object3D's transform changes. Rebuilds the model matrix.
         */
        void onTransformChanged() override
        {
            rebuildMatrix();
        }

        /**
         * ObjectObserver implementation - called when the observed Object3D's appearance changes. Currently does nothing.
         */
        void onAppearanceChanged() override
        {
            // TODO: use the new appearance when drawing the object
        }

        void rebuildMatrix()
        {
            Matrix objectMatrix = object->getTransform().toMatrix();

            modelMatrix = objectMatrix;
        }
    };
} // namespace renderer
