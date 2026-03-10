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
        if (str == "OpenPannelExport")
            return Action::OpenPannelExport;
        if (str == "OpenPannelRotation")
            return Action::OpenPannelRotation;
        if (str == "OpenPannelTransform")
            return Action::OpenPannelTransform;
        if (str == "OpenPannelScale")
            return Action::OpenPannelScale;
        if (str == "SelectObject")
            return Action::SelectObject;
        if (str == "MultiSelection")
            return Action::MultiSelection;
        if (str == "OpenPannelCut")
            return Action::OpenPannelCut;
        return Action::Unknown;
    }
} // namespace input
