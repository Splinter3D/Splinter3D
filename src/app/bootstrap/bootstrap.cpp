#include "bootstrap.hpp"

#include "app/app.hpp"
#include "core/utils/data_root.hpp"
#include "core/utils/os_compatibility.hpp"
#include "core/utils/user_dirs.hpp"
#include "platform/locale/locale_manager.hpp"
#include "platform/logger/logger.hpp"
#include "platform/settings/settings_registry.hpp"
#include "platform/threading/dispatcher.hpp"
#include "ui/layouts/layout_registration.hpp"
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
            "[bootstrap] initializing locale from ", (dataRoot / "assets" / "locale").string());
        platform::locale::LocaleManager::get().init((dataRoot / "assets" / "locale").string());

        // Per user, outside the install (which may be read-only): e.g.
        // ~/Library/Application Support/Splinter3D/settings.cfg.
        // $SPLINTER3D_SETTINGS overrides it (the end-to-end tests point it at a temporary file).
        auto&                 settings     = platform::settings::SettingsRegistry::getInstance();
        std::filesystem::path settingsPath = core::utils::environmentPath("SPLINTER3D_SETTINGS");
        if (const auto userDir = core::utils::userDataDir(); settingsPath.empty() && !userDir.empty())
            settingsPath = userDir / "settings.cfg";
        if (!settingsPath.empty())
        {
            platform::logger::clog("[bootstrap] loading settings from ", settingsPath.string());
            settings.load(settingsPath);
        }
        else
            platform::logger::clog("[bootstrap] no per-user folder: settings will not be saved");
        platform::locale::LocaleManager::get().useSettings(settings);

        const auto profiles =
            platform::threading::Dispatcher::locateProfiles(core::utils::userLocalDataDir(), dataRoot);
        platform::logger::clog("[bootstrap] loading scalability profiles from ", profiles.string());
        platform::threading::Dispatcher::getInstance().load(profiles);

        core::utils::InstallSignalHandlers();
        core::utils::disableCtrlCEcho();

        // Populates every *Registry (PanelRegistry, ToolBarRegistry,
        // MenuRegistry, LayoutRegistry) with their factories. Must run
        // before any window is created, since a window's
        // InitMenuBar()/InitToolBars()/InitLayout()/InitPanels() may query
        // these registries as soon as it's constructed.
        platform::logger::clog("[bootstrap] registering menus");
        ui::menus::RegisterAllMenus();

        platform::logger::clog("[bootstrap] registering toolbars");
        ui::toolbars::RegisterAllToolBars();

        platform::logger::clog("[bootstrap] registering layouts");
        ui::layouts::RegisterAllLayouts();

        platform::logger::clog("[bootstrap] registering panels");
        ui::panels::RegisterAllPanels();

        platform::logger::clog("[bootstrap] creating main window");
        auto mainWindow = std::make_unique<ui::windows::MainWindow>();

        return std::make_unique<App>(std::move(mainWindow));
    }
} // namespace app
