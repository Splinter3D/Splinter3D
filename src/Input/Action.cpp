#include <Input/Action.hpp>

namespace input
{
    Action actionFromString(const std::string& str)
    {
        if (str == "DeleteSelectedObject")
            return Action::DeleteSelectedObject;
        if (str == "DuplicateSelectedObject")
            return Action::DuplicateSelectedObject;
        if (str == "Import")
            return Action::Import;
        if (str == "OpenExportPannel")
            return Action::OpenExportPannel;
        if (str == "OpenPannelRotation")
            return Action::OpenPannelRotation;
        if (str == "OpenPannelTransform")
            return Action::OpenPannelTransform;
        if (str == "OpenPannelScale")
            return Action::OpenPannelScale;
        return Action::Unknown;
    }
} // namespace input