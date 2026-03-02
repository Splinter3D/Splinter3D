#include "renderer/RMesh.hpp"

RMesh RMesh::makeRenderable(const Object3D::OMesh& mesh, Color color)
{
    RMesh result;
    for (const auto& tri : mesh.triangles) {
        result.triangles.push_back({
            .v0 = {tri.v0.x, tri.v0.y, tri.v0.z},
            .v1 = {tri.v1.x, tri.v1.y, tri.v1.z},
            .v2 = {tri.v2.x, tri.v2.y, tri.v2.z},
            .color = color
        });
    }
    return result;
}
