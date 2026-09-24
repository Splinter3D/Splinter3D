#include <StlAPI_Reader.hxx>
#include <geometry/occt/shape.hpp>
#include <stdexcept>
#include <string>
#include <utility>

namespace geometry::occt
{
    Shape Shape::fromSTL(const std::filesystem::path& path)
    {
        const std::string filename = path.string();
        if (filename.empty())
            throw std::invalid_argument("Cannot import an STL shape from an empty path");

        TopoDS_Shape  value;
        StlAPI_Reader reader;
        if (!reader.Read(value, filename.c_str()) || value.IsNull())
            throw std::runtime_error("Failed to import STL shape: " + filename);

        return Shape(std::move(value));
    }
} // namespace geometry::occt
