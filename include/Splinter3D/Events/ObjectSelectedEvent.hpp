#pragma once

namespace scene::events
{
    struct ObjectSelectedEvent
    {
        int index; // -1 = deselected
    };
} // namespace scene::events