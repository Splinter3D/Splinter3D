#include <Gui/Utils/FileDialog.hpp>

namespace gui::utils
{
    std::optional<std::string> pickSTLFile()
    {
        NFD::Guard      guard;
        NFD::UniquePath outPath;
        nfdfilteritem_t filters[] = {{"STL File", "stl"}};

        nfdresult_t result = NFD::OpenDialog(outPath, filters, 1);
        if (result == NFD_OKAY)
            return std::string(outPath.get());
        return std::nullopt;
    }

    std::optional<std::string> saveSTLFile()
    {
        NFD::Guard      guard;
        NFD::UniquePath outPath;
        nfdfilteritem_t filters[] = {{"STL File", "stl"}};

        nfdresult_t result = NFD::SaveDialog(outPath, filters, 1, nullptr, "model.stl");
        if (result == NFD_OKAY)
            return std::string(outPath.get());
        return std::nullopt;
    }
} // namespace gui::utils
