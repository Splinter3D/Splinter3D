#include "bootstrap.hpp"

#include "app/app.hpp"
#include "core/utils/data_root.hpp"
#include "core/utils/os_compatibility.hpp"
#include "platform/locale/locale_manager.hpp"
#include "platform/logger/logger.hpp"
#include "ui/menus/menu_registration.hpp"
#include "ui/panels/panel_registration.hpp"
#include "ui/toolbars/toolbar_registration.hpp"
#include "ui/windows/main_window/main_window.hpp"

#include <filesystem>
#include <wx/stdpaths.h>

namespace app
{
    namespace
    {
        /**
         * @brief Resolves the current executable's absolute path.
         *
         * Relies on wxStandardPaths, which transparently handles
         * Windows, Linux, and macOS - avoiding manual platform-specific
         * lookups such as readlink("/proc/self/exe").
         *
         * @return The absolute path to the running executable.
         */
        std::filesystem::path resolveExecutablePath()
        {
            wxString exe = wxStandardPaths::Get().GetExecutablePath();
            return std::filesystem::path(exe.ToStdString(wxConvUTF8));
        }
    } // namespace

    std::unique_ptr<App> Bootstrap::build(int /*argc*/, char** /*argv*/)
    {
        platform::logger::logger::setDebug(true);
        platform::logger::clog("[bootstrap] startup");

        std::filesystem::path exePath = resolveExecutablePath();

        if (!exePath.empty())
        {
            std::error_code ec;
            std::filesystem::current_path(exePath.parent_path(), ec);
        }

        std::filesystem::path dataRoot = core::utils::findDataRoot(exePath);
        if (dataRoot.empty())
            dataRoot = std::filesystem::current_path();

        platform::logger::clog(
            "[bootstrap] initializing locale from ", (dataRoot / "locale").string());
        platform::locale::LocaleManager::get().init((dataRoot / "locale").string());

        core::utils::InstallSignalHandlers();
        core::utils::disableCtrlCEcho();

        // Populates every *Registry (PanelRegistry, ToolBarRegistry,
        // MenuRegistry) with their factories. Must run before any window is
        // created, since a window's InitMenuBar()/InitToolBars()/InitPanels()
        // may query these registries as soon as it's constructed.
        platform::logger::clog("[bootstrap] registering menus");
        ui::menus::RegisterAllMenus();

        platform::logger::clog("[bootstrap] registering toolbars");
        ui::toolbars::RegisterAllToolBars();

        platform::logger::clog("[bootstrap] registering panels");
        ui::panels::RegisterAllPanels();

        platform::logger::clog("[bootstrap] creating main window");
        auto mainWindow = std::make_unique<ui::windows::MainWindow>();

        return std::make_unique<App>(std::move(mainWindow));
    }
} // namespace app
