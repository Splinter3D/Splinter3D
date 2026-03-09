#pragma once

#include <vector>

namespace splinter3D::events
{
    struct ObjectSelectedEvent
    {
        int              index;   // -1 = deselected, primary selection
        std::vector<int> indices; // complete current selection snapshot
    };
} // namespace splinter3D::events
