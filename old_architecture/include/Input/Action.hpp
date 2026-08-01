#pragma once

#include <cstdint>
#include <string>

namespace input
{
    enum class Action : std::uint8_t
    {
        DeleteSelectedObject    = 0,
        DuplicateSelectedObject = 1,
        Import                  = 2,
        OpenPannelExport        = 3,
        OpenPannelRotation      = 4,
        OpenPannelTransform     = 5,
        OpenPannelScale         = 6,
        OpenPannelCut           = 7,
        Unknown                 = 255
    };

    Action actionFromString(const std::string& str);
} // namespace input
