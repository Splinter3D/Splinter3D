#pragma once

#include <string>

namespace input
{
    enum class Action
    {
        DeleteSelectedObject,
        DuplicateSelectedObject,
        Import,
        OpenExportPannel,
        OpenPannelRotation,
        OpenPannelTransform,
        OpenPannelScale,
        Unknown
    };

    Action actionFromString(const std::string& str);
} // namespace input
