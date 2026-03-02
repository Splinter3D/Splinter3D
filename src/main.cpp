/*
** EPITECH PROJECT, 2026
** Prototype
** File description:
** main
*/

#include <Objects3D/Object3D.hpp>
#include <Renderer/RaylibRenderer.hpp>
#include <Renderer/RenderObject.hpp>
#include <iostream>

int main()
{
    renderer::Config         cfg{1280, 720, "Prototype 3D Slicer", 60};
    renderer::RaylibRenderer renderer(cfg);

    objects3D::Object3D obj = objects3D::Object3D::fromSTL("assets/stl/binary/cube.stl");
    renderer::RenderObject rObj;
    rObj.bind(obj);

    while (!renderer.shouldClose())
    {
        float dt = renderer.beginFrame();

        renderer.updateCamera(dt);

        // Start 3D mode
        renderer.begin3D();

        if (obj.transform.scale.x < 2.0f)
            obj.transform.scale = obj.transform.scale + geometry::Vec3(0.01f, 0.01f, 0.01f);
        else
            obj.transform.scale = geometry::Vec3(1.0f, 1.0f, 1.0f);
        obj.notifyTransform();

        renderer.drawGrid(10, 1.0f);
        renderer.drawAxis(2.0f);
        renderer.drawObject(rObj, {255, 0, 0, 255});

        renderer.end3D();
        renderer.endFrame();
    }

    return 0;
}
