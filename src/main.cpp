/*
** EPITECH PROJECT, 2026
** Prototype
** File description:
** main
*/

#include <Objects3D/OMesh.hpp>
#include <Renderer/RaylibRenderer.hpp>
#include <iostream>

int main()
{
    renderer::Config         cfg{1280, 720, "Prototype 3D Slicer", 60};
    renderer::RaylibRenderer renderer(cfg);

    objects3D::OMesh mesh  = objects3D::OMesh::fromSTL("assets/stl/ascii/cube.stl");
    renderer::RMesh  rmesh = renderer::RMesh::fromOMesh(&mesh, {255, 0, 0, 255});

    while (!renderer.shouldClose())
    {
        float dt = renderer.beginFrame();

        renderer.updateCamera(dt);

        // Start 3D mode
        renderer.begin3D();

        renderer.drawGrid(10, 1.0f);
        renderer.drawAxis(2.0f);
        renderer.drawMesh(rmesh);

        renderer.end3D();
        renderer.endFrame();
    }

    return 0;
}
