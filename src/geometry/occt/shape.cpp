#include <BRepBuilderAPI_MakeShapeOnMesh.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <Message_ProgressScope.hxx>
#include <RWStl.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <geometry/occt/path.hpp>
#include <geometry/occt/shape.hpp>
#include <stdexcept>
#include <string>
#include <utility>

namespace geometry::occt
{
    namespace
    {
        // A copy of mesh with float nodes: STL stores floats, so nothing is
        // lost, and the nodes take half the memory. Precision can only be
        // chosen before the nodes are allocated, hence the copy.
        occ::handle<Poly_Triangulation> singlePrecision(const occ::handle<Poly_Triangulation>& mesh)
        {
            if (!mesh->IsDoublePrecision())
                return mesh;
            occ::handle<Poly_Triangulation> copy = new Poly_Triangulation();
            copy->SetDoublePrecision(false);
            copy->ResizeNodes(mesh->NbNodes(), false);
            copy->ResizeTriangles(mesh->NbTriangles(), false);
            for (int i = 1; i <= mesh->NbNodes(); ++i)
                copy->SetNode(i, mesh->Node(i));
            for (int i = 1; i <= mesh->NbTriangles(); ++i)
                copy->SetTriangle(i, mesh->Triangle(i));
            return copy;
        }
    } // namespace

    Shape Shape::fromSTL(const std::filesystem::path& path,
                         const Message_ProgressRange& progress,
                         const ParallelFor&           parallelFor)
    {
        const std::string filename = toUtf8(path);
        if (filename.empty())
            throw std::invalid_argument("Cannot import an STL shape from an empty path");
        if (isBinarySTL(path))
            return fromMesh(readBinarySTL(path, progress, parallelFor));

        Message_ProgressScope                 scope(progress, "Import STL", 1);
        const occ::handle<Poly_Triangulation> mesh = RWStl::ReadFile(filename.c_str(), scope.Next());
        if (scope.UserBreak())
            throw std::runtime_error("STL import cancelled: " + filename);
        if (mesh.IsNull())
            throw std::runtime_error("Failed to import STL shape: " + filename);

        return fromMesh(singlePrecision(mesh));
    }

    Shape Shape::fromMesh(occ::handle<Poly_Triangulation> mesh)
    {
        TopoDS_Face  face;
        BRep_Builder builder;
        builder.MakeFace(face, mesh);
        return Shape(face);
    }

    occ::handle<Poly_Triangulation> Shape::mesh() const
    {
        if (value_.IsNull() || value_.ShapeType() != TopAbs_FACE)
            return {};
        const TopoDS_Face& face = TopoDS::Face(value_);
        TopLoc_Location    location;
        if (!BRep_Tool::Surface(face, location).IsNull())
            return {};
        return BRep_Tool::Triangulation(face, location);
    }

    int Shape::triangleCount() const
    {
        int count = 0;
        for (TopExp_Explorer it(value_, TopAbs_FACE); it.More(); it.Next())
        {
            TopLoc_Location location;
            const auto      triangulation = BRep_Tool::Triangulation(TopoDS::Face(it.Current()), location);
            if (!triangulation.IsNull())
                count += triangulation->NbTriangles();
        }
        return count;
    }

    Shape Shape::toBRep(const Shape& mesh, const Message_ProgressRange& progress)
    {
        const auto triangulation = mesh.mesh();
        if (triangulation.IsNull())
            throw std::invalid_argument("Only a mesh shape can be converted to a B-Rep");

        Message_ProgressScope          scope(progress, "Convert to B-Rep", 1);
        BRepBuilderAPI_MakeShapeOnMesh converter(triangulation);
        converter.Build(scope.Next());
        if (scope.UserBreak())
            throw std::runtime_error("B-Rep conversion cancelled");
        if (!converter.IsDone() || converter.Shape().IsNull())
            throw std::runtime_error("B-Rep conversion failed");
        return Shape(converter.Shape());
    }
} // namespace geometry::occt
