#pragma once

#include <string>

namespace input
{
    enum class Action
    {
        DeleteSelectedObject,
        DuplicateSelectedObject,
        Import,
        OpenPannelExport,
        OpenPannelRotation,
        OpenPannelTransform,
        OpenPannelScale,
        OpenPannelCut,
        Unknown
    };

    Action actionFromString(const std::string& str);
} // namespace input