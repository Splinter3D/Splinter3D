#pragma once

#include <Geometry/Mesh.hpp>
#include <Objects3D/ObjectObserver.hpp>
#include <Objects3D/Transform.hpp>
#include <Renderer/Color.hpp>
#include <algorithm>
#include <vector>

namespace objects3D
{
    struct Object3D
    {
        geometry::Mesh* mesh;
        Transform       transform;
        renderer::Color color;

        Object3D(geometry::Mesh* m, const renderer::Color& c = {255, 255, 255, 255})
            : mesh(m), color(c)
        { }

        inline static Object3D fromSTL(const std::string& stlFile, const renderer::Color& c = {255, 255, 255, 255})
        {
            return Object3D(new geometry::Mesh(geometry::Mesh::fromSTL(stlFile)), c);
        }

        void attach(ObjectObserver* obs)
        {
            if (std::find(observers.begin(), observers.end(), obs) == observers.end())
                observers.push_back(obs);
        }

        void notifyTransform()
        {
            for (auto* o : observers)
                o->onTransformChanged();
        }

      private:
        std::vector<ObjectObserver*> observers;
    };
} // namespace objects3D