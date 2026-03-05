#include <Geometry/Ray.hpp>
#include <Renderer/IRenderer.hpp>
#include <Scene/SceneObject.hpp>

namespace scene
{
    void SceneObject::draw(renderer::IRenderer& renderer) const
    {
        rObj.draw(renderer);
    }

    bool SceneObject::isHit(const geometry::Ray& ray)
    {
        for (const auto& tri : obj.getTransformedMesh()->triangles)
        {
            const geometry::Vec3 edge1 = tri.vertices[1] - tri.vertices[0];
            const geometry::Vec3 edge2 = tri.vertices[2] - tri.vertices[0];
            const geometry::Vec3 h     = geometry::Vec3::cross(ray.direction, edge2);
            const float          a     = geometry::Vec3::dotProduct(edge1, h);

            if (a > -1e-6f && a < 1e-6f)
                continue;

            const float          f = 1.0f / a;
            const geometry::Vec3 s = ray.origin - tri.vertices[0];
            const float          u = f * geometry::Vec3::dotProduct(s, h);

            if (u < 0.0f || u > 1.0f)
                continue;

            const geometry::Vec3 q = geometry::Vec3::cross(s, edge1);
            const float          v = f * geometry::Vec3::dotProduct(ray.direction, q);

            if (v < 0.0f || u + v > 1.0f)
                continue;

            const float t = f * geometry::Vec3::dotProduct(edge2, q);
            if (t > 1e-6f)
            {
                return true;
            }
        }
        return false;
    }

    objects3D::Transform SceneObject::getTransform() const
    {
        return obj.getTransform();
    }

    void SceneObject::setTransform(const objects3D::Transform& transform)
    {
        obj.setTransform(transform);
    }

    void SceneObject::setColor(const renderer::Color& c)
    {
        rObj.setColor(c);
    }

    geometry::Mesh* SceneObject::getTransformedMesh() const
    {
        return obj.getTransformedMesh();
    }
} // namespace scene