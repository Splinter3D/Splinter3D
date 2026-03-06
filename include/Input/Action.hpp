#pragma once

#include <string>

namespace input
{
    enum class Action
    {
        DeleteSelectedObject,
        DuplicateSelectedObject,
        Import,
        Export,
        OpenPannelRotation,
        OpenPannelTransform,
        OpenPannelScale,
        Unknown
    };

    Action actionFromString(const std::string& str);
} // namespace input