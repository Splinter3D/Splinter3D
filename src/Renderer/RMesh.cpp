#include <Renderer/RMesh.hpp>
#include <Objects3D/OMesh.hpp>

namespace renderer
{

    RMesh renderer::RMesh::fromOMesh(objects3D::OMesh* mesh, Color color)
    {
        RMesh result;
        result.setMeshColor(color);
        result.attachToOMesh(mesh);
        result.loadTrianglesFromOMesh(mesh);
        return result;
    }

    void RMesh::updateFromOMesh()
    {
        if (!_parentMesh) return;

        triangles.clear();
        loadTrianglesFromOMesh(_parentMesh);
    }

    void RMesh::attachToOMesh(objects3D::OMesh* mesh)
    {
        if (_parentMesh)
            detachFromOMesh();
        _parentMesh = mesh;
        if (mesh)
            mesh->attachRenderer(this);
    }

    void RMesh::detachFromOMesh()
    {
        if (_parentMesh)
        {
            _parentMesh->detachRenderer(this);
            _parentMesh = nullptr;
        }
    }

    void RMesh::setMeshColor(Color color)
    {
        _color = color;
        for (auto& tri : triangles)
            tri.color = color;
    }

    void RMesh::loadTrianglesFromOMesh(objects3D::OMesh* mesh)
    {
        triangles.clear();
        for (const auto& tri : mesh->triangles)
        {
            triangles.push_back({.v0    = {tri.vertices[0].x, tri.vertices[0].y, tri.vertices[0].z},
                .v1    = {tri.vertices[1].x, tri.vertices[1].y, tri.vertices[1].z},
                .v2    = {tri.vertices[2].x, tri.vertices[2].y, tri.vertices[2].z},
                .color = _color});
        }
    }
}
