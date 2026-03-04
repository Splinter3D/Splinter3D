#pragma once

namespace objects3D
{
    /**
     * Observer interface for Object3D changes.
     * RenderObjects will implement this to receive updates when the Object3D they are observing changes
     */
    struct ObjectObserver
    {
        virtual ~ObjectObserver()          = default;
        virtual void onTransformChanged()  = 0;
        virtual void onAppearanceChanged() = 0;
    };
} // namespace objects3D
