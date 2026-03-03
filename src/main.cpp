/*
** EPITECH PROJECT, 2026
** Prototype
** File description:
** main
*/

#include <Geometry/Utils/MeshUtils.hpp>
#include <Gui/CenterredToolBar.hpp>
#include <Objects3D/Object3D.hpp>
#include <Renderer/RaylibRenderer.hpp>
#include <Renderer/RenderObject.hpp>

#define RAYGUI_IMPLEMENTATION
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#pragma GCC diagnostic ignored "-Wcast-qual"
#endif
#include <Renderer/RayGUI.hpp>
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

int main()
{
    renderer::Config         cfg{1280, 720, "Prototype 3D Slicer", 60};
    renderer::RaylibRenderer renderer(cfg);

    objects3D::Object3D    obj = objects3D::Object3D::fromSTL("assets/stl/binary/magnolia_binary.stl");
    renderer::RenderObject rObj;
    rObj.bind(obj);

    auto meshBounds = geometry::meshutils::computeMeshBounds(*obj.mesh);

    geometry::meshutils::frameCameraOnMesh(renderer, meshBounds);

    gui::CenteredToolbar toolbar(18.0f, 52.0f, 14.0f);

    while (!renderer.shouldClose())
    {
        float dt = renderer.beginFrame();
        renderer.updateCamera(dt);

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
        toolbar.draw();
        renderer.endFrame();
    }
    return 0;
}
