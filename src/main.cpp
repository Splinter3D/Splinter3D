/*
** EPITECH PROJECT, 2026
** Prototype
** File description:
** main
*/

#ifndef RAYLIB_NO_WINDOWS_H
#define RAYLIB_NO_WINDOWS_H
#endif

#include <Geometry/Utils/MeshUtils.hpp>
#define RAYGUI_IMPLEMENTATION
#include <Gui/CenterredToolBar.hpp>
#include <Objects3D/Object3D.hpp>
#include <Renderer/RaylibRenderer.hpp>
#include <Renderer/RenderObject.hpp>
#include <Splinter3D/Utils/DataRoot.hpp>
#include <Splinter3D/Utils/Locale.hpp>
#include <Splinter3D/Utils/OSCompatibility.hpp>
#include <filesystem>
#include <iostream>

#if !defined(_WIN32)
#include <limits.h>
#include <unistd.h>
#endif

#define _(String) gettext(String)

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
    std::filesystem::path dataRoot = splinter::utils::findDataRoot(exePath);
    if (dataRoot.empty())
        dataRoot = std::filesystem::current_path();

    // Initialize localization (point to the detected locale folder)
    splinter::utils::Locale::init("splinter3D", (dataRoot / "locale").string().c_str());

    // Install cross-platform signal handlers and suppress ^C echo on POSIX
    splinter::utils::oscompat::InstallSignalHandlers();
    splinter::utils::oscompat::disableCtrlCEcho();

    // Example UI strings (ensure gettext is wired)
    std::cout << _("Play") << std::endl;
    std::cout << _("Settings") << std::endl;
    std::cout << _("Quit") << std::endl;

    // Setup renderer
    renderer::Config         cfg{1280, 720, "Prototype 3D Slicer", 60};
    renderer::RaylibRenderer renderer(cfg);

    // Load object and bind to renderer (resolve path relative to data root)
    std::filesystem::path  stlPath = dataRoot / "assets" / "stl" / "binary" / "magnolia_binary.stl";
    objects3D::Object3D    obj     = objects3D::Object3D::fromSTL(stlPath.string());
    renderer::RenderObject rObj;
    rObj.bind(obj);

    auto meshBounds = geometry::meshutils::computeMeshBounds(*obj.mesh);
    geometry::meshutils::frameCameraOnMesh(renderer, meshBounds);

    gui::CenteredToolbar toolbar(18.0f, 52.0f, 14.0f);

    while (!renderer.shouldClose() && !splinter::utils::oscompat::SignalReceived())
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

    // restore terminal state if needed
    splinter::utils::oscompat::restoreTerminal();
    return 0;
}
