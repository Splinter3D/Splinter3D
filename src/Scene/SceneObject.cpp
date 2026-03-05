#include <Renderer/IRenderer.hpp>
#include <Scene/SceneObject.hpp>

namespace scene
{
    void SceneObject::draw(renderer::IRenderer& renderer) const
    {
        rObj.draw(renderer);
    }
} // namespace scene