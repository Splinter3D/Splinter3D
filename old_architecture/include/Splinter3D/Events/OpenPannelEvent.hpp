#pragma once
#include <string>

namespace splinter3D::events
{
    struct OpenPannelEvent
    {
        std::string buttonId; // "rotation", "transform", "scale"
    };
} // namespace splinter3D::events
