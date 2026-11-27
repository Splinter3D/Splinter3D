#pragma once

#include "geometry/occt/windows_prelude.hpp"

#include <Message_ProgressRange.hxx>
#include <Poly_Triangulation.hxx>
#include <cstddef>
#include <filesystem>
#include <functional>

namespace geometry::occt
{
    /**
     * @brief Runs body(i) for every i in [first, last), possibly on several
     * threads; returns when all calls are done. Empty: a plain loop.
     *
     * geometry/ depends on no other layer, so the caller passes its own (the
     * import task passes platform::threading::Dispatcher::parallelFor).
     */
    using ParallelFor = std::function<void(std::size_t first, std::size_t last, const std::function<void(std::size_t)>& body)>;

    /**
     * @brief Whether path is a single-block binary STL: exactly 84 bytes of
     * header and count, then 50 bytes per triangle the count announces.
     * Anything else (ASCII, several solids, trailing bytes) is left to RWStl.
     */
    [[nodiscard]] bool isBinarySTL(const std::filesystem::path& path);

    /**
     * @brief Reads a binary STL (see isBinarySTL()) into a single-precision
     * triangulation, the work spread with parallelFor.
     *
     * The result is RWStl::ReadFile's, node for node: vertices with the same
     * coordinates (bit for bit) become one node, numbered in order of first
     * appearance; triangles left with two equal nodes are dropped; the
     * normals stored in the file are ignored.
     *
     * @throws std::runtime_error if the file can't be mapped, is not a
     * single-block binary STL, holds no triangle, or the read is cancelled
     * (user break on progress, checked between phases).
     */
    [[nodiscard]] occ::handle<Poly_Triangulation> readBinarySTL(const std::filesystem::path& path,
                                                                const Message_ProgressRange& progress    = Message_ProgressRange(),
                                                                const ParallelFor&           parallelFor = {});
} // namespace geometry::occt
