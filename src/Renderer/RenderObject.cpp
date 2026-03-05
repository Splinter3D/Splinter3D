#include <Renderer/IRenderer.hpp>
#include <Renderer/RenderObject.hpp>

namespace renderer
{
    RenderObject::RenderObject(Color color) : _color(color)
    { }

    void RenderObject::bind(objects3D::Object3D& obj)
    {
        object = &obj;
        obj.attach(this);
        rebuildMatrix();
    }

    void RenderObject::draw(renderer::IRenderer& renderer) const
    {
        if (object)
            renderer.drawObject(*this, _color);
    }

    void RenderObject::setColor(const Color& c)
    {
        _color = c;
    }

    void RenderObject::onTransformChanged()
    {
        rebuildMatrix();
    }

    void RenderObject::rebuildMatrix()
    {
        modelMatrix = object->getTransform().toMatrix();
    }

    geometry::Mesh* RenderObject::getTransformedMesh() const
    {
        if (!object)
            return nullptr;

        geometry::Mesh* mesh = object->getMesh();
        if (!mesh)
            return nullptr;

        // Create a transformed copy of the mesh
        geometry::Mesh* transformedMesh = new geometry::Mesh(*mesh);
        for (auto& tri : transformedMesh->triangles)
        {
            for (int i = 0; i < 3; ++i)
            {
                geometry::Vec3& v  = tri.vertices[i];
                Vector3         vt = Vector3{v.x, v.y, v.z};
                vt                 = Vector3Transform(vt, modelMatrix);
                v.x                = vt.x;
                v.y                = vt.y;
                v.z                = vt.z;
            }
        }
        return transformedMesh;
    }
} // namespace renderer