/*
** EPITECH PROJECT, 2026
** Prototype
** File description:
** main
*/

#include <iostream>
#include "renderer/RaylibRenderer.hpp"

int main()
{
    // Initialize renderer with desired configuration
    renderer::Config cfg{1280, 720, "Prototype 3D Slicer", 60};
    renderer::RaylibRenderer renderer(cfg);

    // Simple triangle to test drawing
    renderer::RTriangle tri{
        {0, 0, 0},      // v0
        {1, 0, 0},      // v1
        {0, 1, 0},      // v2
        renderer::Color{255, 0, 0, 255} // Red
    };

    while (!renderer.shouldClose())
    {
        float dt = renderer.beginFrame();

        renderer.updateCamera(dt);

        // Start 3D mode
        renderer.begin3D();

        renderer.drawGrid(10, 1.0f);
        renderer.drawAxis(2.0f);
        renderer.drawTriangle(tri);

        renderer.end3D();
        renderer.endFrame();
    }

    return 0;
}