#include <Geometry/Mesh.hpp>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace geometry
{
    Mesh Mesh::fromSTL(const std::string& filename)
    {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open())
            throw std::runtime_error("Failed to open STL file: " + filename);

        char buf[5];
        file.read(buf, 5);
        file.seekg(0); // rewind to the start for correct parsing

        if (std::string(buf, 5) == "solid")
        {
            return fromAsciiSTL(file);
        }
        else
        {
            return fromBinarySTL(file);
        }
    }

    Mesh Mesh::fromAsciiSTL(std::ifstream& file)
    {
        Mesh        mesh;
        Triangle    currentTri;
        int         vertexCount = 0;
        std::string line;

        while (std::getline(file, line))
        {
            // Look for lines that define vertices and ignore other lines
            if (line.find("vertex") != std::string::npos)
            {
                float              x, y, z;
                std::istringstream iss(line);
                std::string        keyword;
                iss >> keyword >> x >> y >> z;

                if (keyword != "vertex")
                    continue; // skip malformed lines

                currentTri.vertices[vertexCount] = Vec3(x, y, z);
                vertexCount++;

                if (vertexCount == 3)
                {
                    mesh.triangles.push_back(currentTri);
                    vertexCount = 0;
                }
            }
        }

        if (vertexCount != 0)
        {
            throw std::runtime_error("Malformed ASCII STL: leftover vertices");
        }

        return mesh;
    }

    Mesh Mesh::fromBinarySTL(std::ifstream& file)
    {
        Mesh mesh;

        // Read and ignore the 80-byte header
        char header[80];
        file.read(header, 80);

        // Read the number of triangles
        uint32_t triCount = 0;
        file.read(reinterpret_cast<char*>(&triCount), sizeof(uint32_t));
        mesh.triangles.reserve(triCount);

        // Read each triangle
        for (uint32_t i = 0; i < triCount; ++i)
        {
            // Read the normal vector (ignored in this implementation)
            float normal[3];
            if (!file.read(reinterpret_cast<char*>(normal), sizeof(normal)))
                throw std::runtime_error("Unexpected EOF reading normal");

            // Read the vertices
            Triangle tri;
            float    vertex[3];
            for (int v = 0; v < 3; ++v)
            {
                if (!file.read(reinterpret_cast<char*>(vertex), sizeof(vertex)))
                    throw std::runtime_error("Unexpected EOF reading vertex");
                tri.vertices[v] = Vec3(vertex[0], vertex[1], vertex[2]);
            }

            // Read and ignore the 2-byte attribute data
            char attr[2];
            if (!file.read(attr, 2))
                throw std::runtime_error("Unexpected EOF reading attribute bytes");

            mesh.triangles.push_back(tri);
        }

        if (!file)
        {
            throw std::runtime_error("Error reading binary STL: unexpected end of file");
        }
        return mesh;
    }

    bool Mesh::toAsciiSTL(const std::string& filename) const
    {
        std::ofstream out(filename, std::ios::out | std::ios::trunc);
        if (!out.is_open())
            return false;

        out << "solid splinter3d\n";
        out << std::fixed << std::setprecision(6);

        for (const auto& tri : triangles)
        {
            geometry::Vec3 normal = geometry::Vec3::cross(tri.vertices[1] - tri.vertices[0], tri.vertices[2] - tri.vertices[0]);
            float          len    = std::sqrt(geometry::Vec3::dotProduct(normal, normal));
            if (len > 0.0f)
            {
                normal.x /= len;
                normal.y /= len;
                normal.z /= len;
            }
            else
            {
                normal = geometry::Vec3(0.0f, 0.0f, 0.0f);
            }

            out << "  facet normal " << normal.x << ' ' << normal.y << ' ' << normal.z << "\n";
            out << "    outer loop\n";
            for (const auto& vertex : tri.vertices)
                out << "      vertex " << vertex.x << ' ' << vertex.y << ' ' << vertex.z << "\n";
            out << "    endloop\n";
            out << "  endfacet\n";
        }

        out << "endsolid splinter3d\n";
        return true;
    }

    bool Mesh::toBinarySTL(const std::string& filename) const
    {
        std::ofstream out(filename, std::ios::binary | std::ios::out | std::ios::trunc);
        if (!out.is_open())
            return false;

        char              header[80] = {0};
        const std::string headerText = "splinter3d";
        for (size_t i = 0; i < headerText.size() && i < sizeof(header); ++i)
            header[i] = headerText[i];

        out.write(header, sizeof(header));

        const uint32_t triCount = static_cast<uint32_t>(triangles.size());
        out.write(reinterpret_cast<const char*>(&triCount), sizeof(uint32_t));

        for (const auto& tri : triangles)
        {
            geometry::Vec3 normal = geometry::Vec3::cross(tri.vertices[1] - tri.vertices[0], tri.vertices[2] - tri.vertices[0]);
            float          len    = std::sqrt(geometry::Vec3::dotProduct(normal, normal));
            if (len > 0.0f)
            {
                normal.x /= len;
                normal.y /= len;
                normal.z /= len;
            }
            else
            {
                normal = geometry::Vec3(0.0f, 0.0f, 0.0f);
            }

            const float normalData[3] = {normal.x, normal.y, normal.z};
            out.write(reinterpret_cast<const char*>(normalData), sizeof(normalData));

            for (const auto& vertex : tri.vertices)
            {
                const float vertexData[3] = {vertex.x, vertex.y, vertex.z};
                out.write(reinterpret_cast<const char*>(vertexData), sizeof(vertexData));
            }

            const char attributeByteCount[2] = {0, 0};
            out.write(attributeByteCount, sizeof(attributeByteCount));
        }

        return out.good();
    }
} // namespace geometry
