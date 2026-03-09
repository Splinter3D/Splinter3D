// Utils/FilePicker.hpp
#pragma once
#include <nfd.hpp>
#include <optional>
#include <string>

namespace gui::utils
{
    std::optional<std::string> pickSTLFile();
    std::optional<std::string> saveSTLFile();
} // namespace gui::utils
