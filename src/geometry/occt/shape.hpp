#pragma once

// Must come before any OCCT header (see the file).
#include "geometry/occt/windows_prelude.hpp"
// clang-format off: keep the prelude first
#include "geometry/occt/stl_reader.hpp"
// clang-format on

#include <Message_ProgressRange.hxx>
#include <Poly_Triangulation.hxx>
#include <TopoDS_Shape.hxx>
#include <filesystem>
#include <optional>
#include <utility>

namespace geometry::occt
{
    /**
     * @brief OpenCascade shape.
     *
     * CAD operations consume this type instead of exposing OpenCascade
     * in the application and UI layers.
     */
    class Shape
    {
      public:
        /** @brief How a mesh stores its nodes: float (half the memory; STL files hold floats) or double. */
        enum class Precision
        {
            Single,
            Double,
        };

        Shape() = default;

        /**
         * @brief Constructs a Shape from an existing TopoDS_Shape.
         *
         * @param value The OpenCascade shape to wrap.
         */
        explicit Shape(TopoDS_Shape value)
            : value_(std::move(value))
        { }

        /**
         * @brief Imports an STL file as a mesh (see fromMesh()), its vertices
         * merged as RWStl does and stored in `precision` (single loses nothing:
         * the file holds floats). Single-block binary files are read by
         * readBinarySTL(), spread with parallelFor; anything else by RWStl.
         *
         * @param path The path to the STL file.
         * @param progress Receives the progress of reading then building; a
         * user break on it cancels the import.
         * @param precision How the mesh stores its nodes.
         * @return The imported Shape.
         * @throws std::invalid_argument for an empty path, std::runtime_error
         * if the file can't be read or the import was cancelled.
         */
        static Shape fromSTL(const std::filesystem::path& path,
                             const Message_ProgressRange& progress    = Message_ProgressRange(),
                             const ParallelFor&           parallelFor = {},
                             Precision                    precision   = Precision::Single);

        /**
         * @brief A mesh shape: one face, with no surface, carrying the whole
         * triangulation. Cheap to build, display and export; operations that
         * need real topology (booleans, analysis) use toBRep() first.
         */
        static Shape fromMesh(occ::handle<Poly_Triangulation> mesh);

        /** @brief The triangulation of a mesh shape (fromMesh()); null for any other shape. */
        [[nodiscard]] occ::handle<Poly_Triangulation> mesh() const;

        /** @brief How a mesh shape stores its nodes; empty for any other shape. */
        [[nodiscard]] std::optional<Precision> precision() const;

        /**
         * @brief A mesh shape with its nodes stored in `precision` (a copy of
         * the triangulation; double to single rounds the coordinates). Any
         * other shape, or a mesh already in that precision, is returned as is.
         */
        static Shape withPrecision(const Shape& shape, Precision precision);

        /** @brief Triangles over every face's triangulation. */
        [[nodiscard]] int triangleCount() const;

        /**
         * @brief Builds the B-Rep of a mesh shape: one planar face per
         * triangle, vertices and edges shared (what StlAPI_Reader builds).
         * Slow and large on big meshes (Lucy, 28 M triangles: ~4.5 min, ~15 GB).
         *
         * @throws std::invalid_argument if `mesh` is not a mesh shape,
         * std::runtime_error if cancelled (user break on progress) or failed.
         */
        static Shape toBRep(const Shape& mesh, const Message_ProgressRange& progress = Message_ProgressRange());

        // Returns true if the shape is null
        [[nodiscard]] bool isNull() const noexcept
        {
            return value_.IsNull();
        }

        // Returns the underlying TopoDS_Shape (non-const version).
        [[nodiscard]] TopoDS_Shape& value() noexcept
        {
            return value_;
        }

        // Returns the underlying TopoDS_Shape (const version).
        [[nodiscard]] const TopoDS_Shape& value() const noexcept
        {
            return value_;
        }

      private:
        TopoDS_Shape value_;
    };
} // namespace geometry::occt
