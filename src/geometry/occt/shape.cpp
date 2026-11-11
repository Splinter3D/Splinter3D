#include <BRepBuilderAPI_MakeShapeOnMesh.hxx>
#include <Message_ProgressScope.hxx>
#include <RWStl.hxx>
#include <geometry/occt/path.hpp>
#include <geometry/occt/shape.hpp>
#include <stdexcept>
#include <string>
#include <utility>

namespace geometry::occt
{
    Shape Shape::fromSTL(const std::filesystem::path& path, const Message_ProgressRange& progress)
    {
        const std::string filename = toUtf8(path);
        if (filename.empty())
            throw std::invalid_argument("Cannot import an STL shape from an empty path");

        // Same steps as StlAPI_Reader::Read (OCCT 8.0), which takes no progress.
        Message_ProgressScope                 scope(progress, "Import STL", 2);
        const occ::handle<Poly_Triangulation> mesh = RWStl::ReadFile(filename.c_str(), scope.Next());
        if (scope.UserBreak())
            throw std::runtime_error("STL import cancelled: " + filename);
        if (mesh.IsNull())
            throw std::runtime_error("Failed to import STL shape: " + filename);

        BRepBuilderAPI_MakeShapeOnMesh converter(mesh);
        converter.Build(scope.Next());
        if (scope.UserBreak())
            throw std::runtime_error("STL import cancelled: " + filename);
        if (!converter.IsDone() || converter.Shape().IsNull())
            throw std::runtime_error("Failed to import STL shape: " + filename);

        return Shape(converter.Shape());
    }
} // namespace geometry::occt
