#include <Objects3D/OMesh.hpp>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include <Renderer/RMesh.hpp>

namespace objects3D
{
    OMesh OMesh::fromSTL(const std::string& filename)
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

    OMesh OMesh::fromAsciiSTL(std::ifstream& file)
    {
        OMesh       mesh;
        OTriangle   currentTri;
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

                currentTri.vertices[vertexCount] = OVec3(x, y, z);
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

    OMesh OMesh::fromBinarySTL(std::ifstream& file)
    {
        OMesh mesh;

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
            OTriangle tri;
            float     vertex[3];
            for (int v = 0; v < 3; ++v)
            {
                if (!file.read(reinterpret_cast<char*>(vertex), sizeof(vertex)))
                    throw std::runtime_error("Unexpected EOF reading vertex");
                tri.vertices[v] = OVec3(vertex[0], vertex[1], vertex[2]);
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

    void OMesh::attachRenderer(renderer::RMesh* rmesh)
    {
        observers.push_back(rmesh);
    }

    void OMesh::detachRenderer(renderer::RMesh* rmesh)
    {
        observers.erase(std::remove(observers.begin(), observers.end(), rmesh), observers.end());
    }

    void OMesh::notifyChange() const
    {
        for (auto* r : observers) r->updateFromOMesh();
    }

    void OMesh::scale(float percentage)
    {
        float factor = percentage / 100.0f;
        for (auto& tri : triangles)
        {
            for (int i = 0; i < 3; ++i)
            {
                tri.vertices[i].x *= factor;
                tri.vertices[i].y *= factor;
                tri.vertices[i].z *= factor;
            }
        }
        notifyChange();
    }

} // namespace objects3D
