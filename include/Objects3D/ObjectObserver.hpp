#pragma once

namespace objects3D
{
    struct ObjectObserver
    {
        virtual ~ObjectObserver() = default;
        virtual void onTransformChanged() = 0;
        virtual void onAppearanceChanged() = 0;
    };
}