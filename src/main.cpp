/*
** EPITECH PROJECT, 2026
** Prototype
** File description:
** main
*/

#include <Objects3D/OMesh.hpp>
#include <Renderer/RaylibRenderer.hpp>
#include <Splinter/Utils/Locale.hpp>
#include <Splinter/Utils/Logger.hpp>
#include <cstdarg>
#include <cstdio>
#include <iostream>
#include <raylib.h>
#include <string>

#define _(String) gettext(String)

static void RaylibToLogger([[maybe_unused]] int         logLevel,
                           [[maybe_unused]] const char* text, [[maybe_unused]] va_list args)
{
#if !defined(SPLINTER_DEBUG)
    return;
#endif

    char* buf = NULL;

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
#endif

    int ret = vasprintf(&buf, text, args);

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

    if (ret < 0)
    {
        return;
    }

    const char* lvl = "INFO";
    switch (logLevel)
    {
        case LOG_TRACE:
            lvl = "TRACE";
            break;
        case LOG_DEBUG:
            lvl = "DEBUG";
            break;
        case LOG_INFO:
            lvl = "INFO";
            break;
        case LOG_WARNING:
            lvl = "WARN";
            break;
        case LOG_ERROR:
            lvl = "ERROR";
            break;
        case LOG_FATAL:
            lvl = "FATAL";
            break;
        default:
            break;
    }

    splinter::utils::clog("[raylib ", lvl, "] ", buf);
    std::free(buf);
}

int main()
{
    splinter::utils::Locale::init("splinter3D", "./locale");

    std::cout << _("Play") << std::endl;
    std::cout << _("Settings") << std::endl;
    std::cout << _("Quit") << std::endl;

    SetTraceLogCallback(RaylibToLogger);
    renderer::Config         cfg{1280, 720, "Prototype 3D Slicer", 60};
    renderer::RaylibRenderer renderer(cfg);

    objects3D::OMesh mesh  = objects3D::OMesh::fromSTL("assets/stl/ascii/cube.stl");
    renderer::RMesh  rmesh = renderer::RMesh::fromOMesh(mesh, {255, 0, 0, 255});

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
