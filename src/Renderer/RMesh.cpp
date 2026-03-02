#include <Renderer/RMesh.hpp>

renderer::RMesh renderer::RMesh::fromOMesh(const objects3D::OMesh& mesh, Color color)
{
    RMesh result;
    for (const auto& tri : mesh.triangles)
    {
        result.triangles.push_back({.v0    = {tri.vertices[0].x, tri.vertices[0].y, tri.vertices[0].z},
                                    .v1    = {tri.vertices[1].x, tri.vertices[1].y, tri.vertices[1].z},
                                    .v2    = {tri.vertices[2].x, tri.vertices[2].y, tri.vertices[2].z},
                                    .color = color});
    }
    return result;
}
