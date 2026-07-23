// /*
// ** EPITECH PROJECT, 2026
// ** Prototype
// ** File description:
// ** main
// */

// #ifndef RAYLIB_NO_WINDOWS_H
// #define RAYLIB_NO_WINDOWS_H
// #endif

// #include <Geometry/Utils/MeshUtils.hpp>
// #include <Gui/CenteredToolbar.hpp>
// #include <Gui/States/ScalePannelState.hpp>
// #include <Input/InputBindings.hpp>
// #include <Input/InputManager.hpp>
// #include <Objects3D/Object3D.hpp>
// #include <Renderer/RaylibRenderer.hpp>
// #include <Renderer/RenderObject.hpp>
// #include <Scene/Scene.hpp>
// #include <Splinter3D/Utils/DataRoot.hpp>
// #include <Splinter3D/Utils/Locale.hpp>
// #include <Splinter3D/Utils/OSCompatibility.hpp>
// #include <cstdlib>
// #include <filesystem>
// #include <iostream>
// #include <raylib.h>

// #if !defined(_WIN32)
// #include <limits.h>
// #include <unistd.h>
// #endif

// int main(int argc, char** argv)
// {
//     splinter3D::utils::logger::setDebug(true);
//     splinter3D::utils::clog("[main] startup");

//     std::filesystem::path exePath;
//     try
//     {
// #if !defined(_WIN32)
//         char    buf[PATH_MAX];
//         ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
//         if (len != -1)
//         {
//             buf[len] = '\0';
//             exePath  = std::filesystem::path(buf);
//         }
// #endif
//         if (exePath.empty() && argc > 0)
//         {
//             exePath = std::filesystem::absolute(argv[0]);
//         }
//         if (!exePath.empty())
//         {
//             std::error_code ec;
//             std::filesystem::current_path(exePath.parent_path(), ec);
//         }
//     }
//     catch (...)
//     { }

//     // Resolve data root (where `locale` and `assets` live).
//     std::filesystem::path dataRoot = splinter3D::utils::findDataRoot(exePath);
//     if (dataRoot.empty())
//         dataRoot = std::filesystem::current_path();

//     // Initialize localization (point to the detected locale folder)
//     splinter3D::utils::clog("[main] initializing locale from ", (dataRoot / "locale").string());
//     splinter3D::utils::Locale::init((dataRoot / "locale").string().c_str());

//     // Install cross-platform signal handlers and suppress ^C echo on POSIX
//     splinter3D::utils::oscompat::InstallSignalHandlers();
//     splinter3D::utils::oscompat::disableCtrlCEcho();
//     splinter3D::utils::clog("[main] creating renderer");

//     // Example UI strings (ensure gettext is wired)
//     // std::cout << splinter3D::utils::Locale::gettext("play") << std::endl;
//     // std::cout << splinter3D::utils::Locale::gettext("settings.title") << std::endl;
//     // std::cout << splinter3D::utils::Locale::gettext("settings.language", {{"lang", splinter3D::utils::Locale::getActiveLanguage()}}) << std::endl;
//     // std::cout << splinter3D::utils::Locale::gettext("settings.test", {{"test", "value"}}) << std::endl;
//     // std::cout << splinter3D::utils::Locale::gettext("settings.nested_flattening.test", {{"test", "nested_value"}}) << std::endl;
//     // std::cout << splinter3D::utils::Locale::gettext("quit") << std::endl;

//     renderer::Config         cfg{1270, 720, "Prototype 3D Slicer", 60};
//     renderer::RaylibRenderer renderer(cfg);
//     splinter3D::utils::clog("[main] renderer created");
//     gui::CenteredToolbar toolbar(18.0f, 52.0f, 14.0f);
//     splinter3D::utils::clog("[main] initializing toolbar");
//     toolbar.initialize(renderer);
//     splinter3D::utils::clog("[main] toolbar initialized");

//     input::registerBindings();
//     splinter3D::utils::clog("[main] input bindings registered");
//     while (!renderer.shouldClose())
//     {
//         float dt = renderer.beginFrame();

//         input::InputManager::getInstance().update(renderer);
//         renderer.updateCamera(dt);
//         toolbar.update(renderer);

//         renderer.begin3D();
//         if (renderer.isMouseButtonPressed((int) renderer::MouseButton::Left))
//         {
//             auto ray = renderer.getMouseRay();
//             scene::Scene::getInstance().handleClick(ray);
//         }
//         renderer.drawGrid(10, 1.0f);
//         renderer.drawAxis(2.0f);
//         scene::Scene::getInstance().draw(renderer);

//         renderer.end3D();
//         renderer.drawGuiComponent(toolbar);
//         renderer.endFrame();
//     }

//     // restore terminal state if needed
//     splinter3D::utils::oscompat::restoreTerminal();
//     return 0;
// }
