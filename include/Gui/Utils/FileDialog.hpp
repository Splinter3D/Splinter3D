// Utils/FilePicker.hpp
#pragma once
#include <nfd.hpp>
#include <optional>
#include <string>

namespace gui::utils
{
    inline std::optional<std::string> pickSTLFile()
    {
        NFD::Guard      guard;
        NFD::UniquePath outPath;
        nfdfilteritem_t filters[] = {{"STL File", "stl"}};

        nfdresult_t result = NFD::OpenDialog(outPath, filters, 1);
        if (result == NFD_OKAY)
            return std::string(outPath.get());
        return std::nullopt;
    }
} // namespace gui::utils