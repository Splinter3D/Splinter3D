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

    void RenderObject::onAppearanceChanged()
    {
        // TODO: use the new appearance when drawing the object
    }

    void RenderObject::rebuildMatrix()
    {
        modelMatrix = object->getTransform().toMatrix();
    }
} // namespace renderer