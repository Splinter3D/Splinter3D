/*
** EPITECH PROJECT, 2026
** Prototype
** File description:
** main
*/

#include <Geometry/Utils/MeshUtils.hpp>
#include <Gui/CenteredToolbar.hpp>
#include <Gui/States/ScalePanelState.hpp>
#include <Objects3D/Object3D.hpp>
#include <Renderer/RaylibRenderer.hpp>
#include <Scene/Scene.hpp>
#include <Splinter3D/Utils/Locale.hpp>

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
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wcast-align"
#if !defined(__clang__)
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#endif
#endif

#include <Renderer/RayGUI.hpp>

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

int main()
{
    splinter3D::utils::Locale::init("splinter3D", "./locale");
    renderer::Config         cfg{1270, 720, "Prototype 3D Slicer", 60};
    renderer::RaylibRenderer renderer(cfg);

    gui::CenteredToolbar toolbar(18.0f, 52.0f, 14.0f);
    toolbar.initialize(renderer);

    // {
    //     auto&                      scaleState = gui::states::ScalePanelState::instance();
    //     const objects3D::Transform t          = obj.getTransform();

    //     scaleState.target  = &obj;
    //     scaleState.scaleX  = t.scale.x * 100.0f;
    //     scaleState.scaleY  = t.scale.y * 100.0f;
    //     scaleState.scaleZ  = t.scale.z * 100.0f;
    //     scaleState.scaleXi = (int) scaleState.scaleX;
    //     scaleState.scaleYi = (int) scaleState.scaleY;
    //     scaleState.scaleZi = (int) scaleState.scaleZ;
    // }

    while (!renderer.shouldClose())
    {
        float dt = renderer.beginFrame();
        renderer.updateCamera(dt);
        toolbar.update(renderer);

        renderer.begin3D();
        if (renderer.isMouseButtonPressed((int) renderer::MouseButton::Left))
        {
            auto ray = renderer.getMouseRay();
            scene::Scene::getInstance().handleClick(ray);
        }
        renderer.drawGrid(10, 1.0f);
        renderer.drawAxis(2.0f);
        scene::Scene::getInstance().draw(renderer);

        renderer.end3D();
        renderer.drawGuiComponent(toolbar);
        renderer.endFrame();
    }
    return 0;
}
