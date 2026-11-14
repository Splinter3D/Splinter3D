#include "mesh_pick_index.hpp"

#include <BRep_Tool.hxx>
#include <TopExp_Explorer.hxx>
#include <TopLoc_Location.hxx>
#include <TopoDS.hxx>
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <utility>

namespace geometry::occt
{
    namespace
    {
        constexpr std::uint32_t kLeafSize = 8;
        // Barycentric slack, so a line through a shared edge or corner hits.
        constexpr double kEdgeSlack = 1e-12;

        float roundedDown(double value)
        {
            return std::nextafter(static_cast<float>(value), -std::numeric_limits<float>::infinity());
        }

        float roundedUp(double value)
        {
            return std::nextafter(static_cast<float>(value), std::numeric_limits<float>::infinity());
        }
    } // namespace

    std::shared_ptr<const MeshPickIndex> MeshPickIndex::build(const Shape& shape)
    {
        if (shape.isNull())
            return nullptr;

        auto          index = std::shared_ptr<MeshPickIndex>(new MeshPickIndex());
        std::uint32_t total = 0;
        for (TopExp_Explorer it(shape.value(), TopAbs_FACE); it.More(); it.Next())
        {
            TopLoc_Location location;
            auto            mesh = BRep_Tool::Triangulation(TopoDS::Face(it.Current()), location);
            if (mesh.IsNull() || mesh->NbTriangles() == 0)
                continue;
            Face face;
            face.mesh     = mesh;
            face.location = location.Transformation();
            face.moved    = !location.IsIdentity();
            face.first    = total;
            total += static_cast<std::uint32_t>(mesh->NbTriangles());
            index->faces_.push_back(std::move(face));
        }
        if (total == 0)
            return nullptr;

        std::vector<float> centroids(static_cast<std::size_t>(total) * 3);
        index->triangles_.resize(total);
        for (std::uint32_t id = 0; id < total; ++id)
        {
            index->triangles_[id] = id;
            gp_XYZ a, b, c;
            index->corners(id, a, b, c);
            const gp_XYZ centre = (a + b + c) / 3.0;
            for (int axis = 0; axis < 3; ++axis)
                centroids[static_cast<std::size_t>(id) * 3 + static_cast<std::size_t>(axis)] = static_cast<float>(centre.Coord(axis + 1));
        }
        index->nodes_.reserve(2 * (total / kLeafSize + 1));
        index->buildNode(0, total, centroids);
        return index;
    }

    void MeshPickIndex::corners(std::uint32_t triangle, gp_XYZ& a, gp_XYZ& b, gp_XYZ& c) const
    {
        // The last face starting at or before this triangle.
        const auto  face = std::upper_bound(faces_.begin(), faces_.end(), triangle,
                                            [](std::uint32_t id, const Face& f) { return id < f.first; }) -
                           1;
        const auto& mesh = face->mesh;
        int         n1   = 0;
        int         n2   = 0;
        int         n3   = 0;
        mesh->Triangle(static_cast<int>(triangle - face->first) + 1).Get(n1, n2, n3);
        gp_Pnt p1 = mesh->Node(n1);
        gp_Pnt p2 = mesh->Node(n2);
        gp_Pnt p3 = mesh->Node(n3);
        if (face->moved)
        {
            p1.Transform(face->location);
            p2.Transform(face->location);
            p3.Transform(face->location);
        }
        a = p1.XYZ();
        b = p2.XYZ();
        c = p3.XYZ();
    }

    void MeshPickIndex::buildNode(std::uint32_t start, std::uint32_t count, const std::vector<float>& centroids)
    {
        const auto self = static_cast<std::uint32_t>(nodes_.size());
        nodes_.emplace_back();

        std::array<double, 3> low{};
        std::array<double, 3> high{};
        std::array<float, 3>  centreLow{};
        std::array<float, 3>  centreHigh{};
        low.fill(std::numeric_limits<double>::infinity());
        high.fill(-std::numeric_limits<double>::infinity());
        centreLow.fill(std::numeric_limits<float>::infinity());
        centreHigh.fill(-std::numeric_limits<float>::infinity());
        for (std::uint32_t i = start; i < start + count; ++i)
        {
            gp_XYZ corner[3];
            corners(triangles_[i], corner[0], corner[1], corner[2]);
            for (int axis = 0; axis < 3; ++axis)
            {
                const auto a = static_cast<std::size_t>(axis);
                for (const gp_XYZ& p : corner)
                {
                    low[a]  = std::min(low[a], p.Coord(axis + 1));
                    high[a] = std::max(high[a], p.Coord(axis + 1));
                }
                const float centre = centroids[static_cast<std::size_t>(triangles_[i]) * 3 + a];
                centreLow[a]       = std::min(centreLow[a], centre);
                centreHigh[a]      = std::max(centreHigh[a], centre);
            }
        }
        for (std::size_t a = 0; a < 3; ++a)
        {
            nodes_[self].min[a] = roundedDown(low[a]);
            nodes_[self].max[a] = roundedUp(high[a]);
        }

        // Split at the median along the axis where the centres spread most.
        std::size_t axis = 0;
        for (std::size_t a = 1; a < 3; ++a)
            if (centreHigh[a] - centreLow[a] > centreHigh[axis] - centreLow[axis])
                axis = a;
        if (count <= kLeafSize || centreHigh[axis] <= centreLow[axis])
        {
            nodes_[self].start = start;
            nodes_[self].count = count;
            return;
        }
        const std::uint32_t half  = count / 2;
        const auto          first = triangles_.begin() + start;
        std::nth_element(first, first + half, first + count, [&](std::uint32_t l, std::uint32_t r) {
            return centroids[static_cast<std::size_t>(l) * 3 + axis] < centroids[static_cast<std::size_t>(r) * 3 + axis];
        });
        buildNode(start, half, centroids);
        nodes_[self].right = static_cast<std::uint32_t>(nodes_.size());
        buildNode(start + half, count - half, centroids);
    }

    double MeshPickIndex::hitTriangle(std::uint32_t triangle, const gp_XYZ& origin, const gp_XYZ& direction, bool& hit) const
    {
        // Möller-Trumbore, on the whole line.
        gp_XYZ a, b, c;
        corners(triangle, a, b, c);
        const gp_XYZ edge1 = b - a;
        const gp_XYZ edge2 = c - a;
        const gp_XYZ p     = direction.Crossed(edge2);
        const double det   = edge1.Dot(p);
        hit                = false;
        if (std::abs(det) < 1e-300)
            return 0.0;
        const double inverse = 1.0 / det;
        const gp_XYZ s       = origin - a;
        const double u       = s.Dot(p) * inverse;
        if (u < -kEdgeSlack || u > 1.0 + kEdgeSlack)
            return 0.0;
        const gp_XYZ q = s.Crossed(edge1);
        const double v = direction.Dot(q) * inverse;
        if (v < -kEdgeSlack || u + v > 1.0 + kEdgeSlack)
            return 0.0;
        hit = true;
        return edge2.Dot(q) * inverse;
    }

    std::optional<double> MeshPickIndex::intersect(const gp_Lin& ray) const
    {
        if (nodes_.empty())
            return std::nullopt;
        const gp_XYZ origin    = ray.Location().XYZ();
        const gp_XYZ direction = ray.Direction().XYZ();
        double       best      = std::numeric_limits<double>::infinity();
        bool         found     = false;

        // Where the line enters a node's box (-infinity..+infinity), or nothing.
        const auto enter = [&](const Node& node) -> std::optional<double> {
            double near = -std::numeric_limits<double>::infinity();
            double far  = std::numeric_limits<double>::infinity();
            for (int axis = 0; axis < 3; ++axis)
            {
                const double o  = origin.Coord(axis + 1);
                const double d  = direction.Coord(axis + 1);
                const auto   lo = static_cast<double>(node.min[axis]);
                const auto   hi = static_cast<double>(node.max[axis]);
                if (d == 0.0)
                {
                    if (o < lo || o > hi)
                        return std::nullopt;
                    continue;
                }
                double t0 = (lo - o) / d;
                double t1 = (hi - o) / d;
                if (t0 > t1)
                    std::swap(t0, t1);
                near = std::max(near, t0);
                far  = std::min(far, t1);
                if (near > far)
                    return std::nullopt;
            }
            return near;
        };

        std::vector<std::uint32_t> stack{0};
        while (!stack.empty())
        {
            const Node& node = nodes_[stack.back()];
            const auto  self = stack.back();
            stack.pop_back();
            const auto entry = enter(node);
            if (!entry || *entry > best)
                continue;
            if (node.count > 0)
            {
                for (std::uint32_t i = node.start; i < node.start + node.count; ++i)
                {
                    bool         hit = false;
                    const double t   = hitTriangle(triangles_[i], origin, direction, hit);
                    if (hit && t < best)
                    {
                        best  = t;
                        found = true;
                    }
                }
                continue;
            }
            stack.push_back(node.right);
            stack.push_back(self + 1);
        }
        return found ? std::optional<double>(best) : std::nullopt;
    }
} // namespace geometry::occt
