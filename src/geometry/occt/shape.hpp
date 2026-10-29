#pragma once

#include <TopoDS_Shape.hxx>
#include <filesystem>
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
         * @brief Imports a shape from an STL file.
         *
         * @param path The path to the STL file.
         * @return The imported Shape.
         */
        static Shape fromSTL(const std::filesystem::path& path);

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
