/*
** EPITECH PROJECT, 2026
** Prototype
** File description:
** main
*/

#ifndef RAYLIB_NO_WINDOWS_H
#define RAYLIB_NO_WINDOWS_H
#endif

#define RAYGUI_IMPLEMENTATION

#include <Geometry/Utils/MeshUtils.hpp>
#include <Gui/CenteredToolbar.hpp>
#include <Gui/States/ScalePannelState.hpp>
#include <Input/InputBindings.hpp>
#include <Input/InputManager.hpp>
#include <Objects3D/Object3D.hpp>
#include <Renderer/RaylibRenderer.hpp>
#include <Renderer/RenderObject.hpp>
#include <Scene/Scene.hpp>
#include <Splinter3D/Utils/DataRoot.hpp>
#include <Splinter3D/Utils/Locale.hpp>
#include <Splinter3D/Utils/OSCompatibility.hpp>
#include <filesystem>
#include <iostream>

#if !defined(_WIN32)
#include <limits.h>
#include <unistd.h>
#endif

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

int main(int argc, char** argv)
{
    std::filesystem::path exePath;
    try
    {
#if !defined(_WIN32)
        char    buf[PATH_MAX];
        ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
        if (len != -1)
        {
            buf[len] = '\0';
            exePath  = std::filesystem::path(buf);
        }
#endif
        if (exePath.empty() && argc > 0)
        {
            exePath = std::filesystem::absolute(argv[0]);
        }
        if (!exePath.empty())
        {
            std::error_code ec;
            std::filesystem::current_path(exePath.parent_path(), ec);
        }
    }
    catch (...)
    { }

    // Resolve data root (where `locale` and `assets` live).
    std::filesystem::path dataRoot = splinter3D::utils::findDataRoot(exePath);
    if (dataRoot.empty())
        dataRoot = std::filesystem::current_path();

    // Initialize localization (point to the detected locale folder)
    splinter3D::utils::Locale::init("splinter3D", (dataRoot / "locale").string().c_str());

    // Install cross-platform signal handlers and suppress ^C echo on POSIX
    splinter3D::utils::oscompat::InstallSignalHandlers();
    splinter3D::utils::oscompat::disableCtrlCEcho();

    // Example UI strings (ensure gettext is wired)
    // std::cout << _("Play") << std::endl;
    // std::cout << _("Settings") << std::endl;
    // std::cout << _("Quit") << std::endl;

    renderer::Config         cfg{1270, 720, "Prototype 3D Slicer", 60};
    renderer::RaylibRenderer renderer(cfg);
    gui::CenteredToolbar     toolbar(18.0f, 52.0f, 14.0f);
    toolbar.initialize(renderer);

    input::registerBindings();
    while (!renderer.shouldClose())
    {
        float dt = renderer.beginFrame();

        input::InputManager::getInstance().update(renderer);
        renderer.updateCamera(dt);
        toolbar.update(renderer);

        renderer.begin3D();
        if (renderer.isMouseButtonPressed((int) renderer::MouseButton::Left) && !toolbar.isMouseOver(renderer))
        {
            auto       ray      = renderer.getMouseRay();
            const bool additive = renderer.isKeyDown(renderer::Key::Ctrl);
            scene::Scene::getInstance().handleClick(ray, additive);
        }
        renderer.drawGrid(10, 1.0f);
        renderer.drawAxis(2.0f);
        scene::Scene::getInstance().draw(renderer);

        renderer.end3D();
        renderer.drawGuiComponent(toolbar);
        renderer.endFrame();
    }

    // restore terminal state if needed
    splinter3D::utils::oscompat::restoreTerminal();
    return 0;
}
