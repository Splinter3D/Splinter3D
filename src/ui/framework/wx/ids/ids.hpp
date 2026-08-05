#pragma once

#include <wx/wx.h>

namespace ui::framework::wx
{
    namespace ids
    {

        /**
         * @brief Standard wxWidgets file operation identifiers.
         *
         * These IDs are provided by wxWidgets and should be reused for common
         * file-related actions.
         *
         * ---
         *
         * Example:
         *
         * @code{.cpp}
         * fileMenu->Append(
         *     wx::ids::file::kOpen,
         *     _("Open")
         * );
         * @endcode
         */
        namespace file
        {
            constexpr int kNew    = wxID_NEW;
            constexpr int kOpen   = wxID_OPEN;
            constexpr int kSave   = wxID_SAVE;
            constexpr int kSaveAs = wxID_SAVEAS;
            constexpr int kClose  = wxID_CLOSE;
            constexpr int kExit   = wxID_EXIT;

        } // namespace file

        /**
         * @brief Standard wxWidgets editing identifiers.
         *
         * These IDs correspond to common text editing operations.
         */
        namespace edit
        {
            constexpr int kUndo      = wxID_UNDO;
            constexpr int kRedo      = wxID_REDO;
            constexpr int kCut       = wxID_CUT;
            constexpr int kCopy      = wxID_COPY;
            constexpr int kPaste     = wxID_PASTE;
            constexpr int kDelete    = wxID_DELETE;
            constexpr int kSelectAll = wxID_SELECTALL;
            constexpr int kFind      = wxID_FIND;
            constexpr int kReplace   = wxID_REPLACE;

        } // namespace edit

        /**
         * @brief Standard wxWidgets help-related identifiers.
         */
        namespace help
        {
            constexpr int kHelp  = wxID_HELP;
            constexpr int kAbout = wxID_ABOUT;

        } // namespace help

        /**
         * @brief Standard wxWidgets dialog identifiers.
         *
         * These IDs are commonly used by dialogs and confirmation actions.
         */
        namespace dialog
        {
            constexpr int kOk     = wxID_OK;
            constexpr int kCancel = wxID_CANCEL;
            constexpr int kApply  = wxID_APPLY;
            constexpr int kYes    = wxID_YES;
            constexpr int kNo     = wxID_NO;

        } // namespace dialog

        /**
         * @brief Base identifier for application-specific events.
         *
         * wxWidgets reserves all identifiers below wxID_HIGHEST.
         * Custom application IDs must start above this value.
         *
         * Reserved ranges:
         *
         * - +000 to +099 : View actions
         * - +100 to +199 : Theme actions
         * - +200 to +299 : Language actions
         * - +300 to +399 : Tool actions
         * - +400 to +499 : Window management
         * - +500 to +599 : Debug actions
         * - +600 to +699 : Panel identifiers
         * - +700 to +799 : Toolbar identifiers
         * - +800 to +899 : Menu identifiers
         */
        namespace app
        {
            constexpr int kBase = wxID_HIGHEST + 1000;

        } // namespace app

        /**
         * @brief View-related application actions.
         *
         * These identifiers control the visual state of the application.
         */
        namespace view
        {
            constexpr int kRefresh          = app::kBase + 0;
            constexpr int kResetLayout      = app::kBase + 1;
            constexpr int kToggleFullscreen = app::kBase + 2;
            constexpr int kShowToolbar      = app::kBase + 3;
            constexpr int kShowStatusbar    = app::kBase + 4;
            constexpr int kShowGrid         = app::kBase + 5;

        } // namespace view

        /**
         * @brief Theme-related application actions.
         */
        namespace theme
        {
            constexpr int kToggleDarkMode = app::kBase + 100;
            constexpr int kLightMode      = app::kBase + 101;
            constexpr int kDarkMode       = app::kBase + 102;

        } // namespace theme

        /**
         * @brief Language selection actions.
         */
        namespace language
        {
            constexpr int kDefault = app::kBase + 200;
            constexpr int kEnglish = app::kBase + 201;
            constexpr int kFrench  = app::kBase + 202;
            constexpr int kGerman  = app::kBase + 203;
            constexpr int kSpanish = app::kBase + 204;

        } // namespace language

        /**
         * @brief Application tool actions.
         */
        namespace tools
        {
            constexpr int kPreferences   = app::kBase + 300;
            constexpr int kSettings      = app::kBase + 301;
            constexpr int kPluginManager = app::kBase + 302;

        } // namespace tools

        /**
         * @brief Window management actions.
         */
        namespace window
        {
            constexpr int kNewWindow      = app::kBase + 400;
            constexpr int kCloseWindow    = app::kBase + 401;
            constexpr int kNextWindow     = app::kBase + 402;
            constexpr int kPreviousWindow = app::kBase + 403;

        } // namespace window

        /**
         * @brief Development and debugging actions.
         *
         * These IDs should normally not be exposed in production builds.
         */
        namespace debug
        {
            constexpr int kReloadTheme    = app::kBase + 500;
            constexpr int kReloadLocale   = app::kBase + 501;
            constexpr int kShowDebugPanel = app::kBase + 502;

        } // namespace debug

        /**
         * @brief Panel identifiers.
         *
         * Used as keys into ui::panels::PanelRegistry (see
         * ui/panels/panel_registry.hpp) - NOT fired as wx command events like
         * the other ranges above. A window passes one of these to
         * PanelRegistry::Instance().Create(id, parent) to instantiate the
         * corresponding panel.
         */
        namespace panels
        {
            constexpr int kExample = app::kBase + 600;

        } // namespace panels

        /**
         * @brief Toolbar identifiers.
         *
         * Used as keys into ui::toolbars::ToolBarRegistry (see
         * ui/toolbars/toolbar_registry.hpp) - NOT fired as wx command events.
         * A window passes one of these to
         * ToolBarRegistry::getInstance().Create(id, parent) to instantiate
         * the corresponding toolbar.
         */
        namespace toolbars
        {
            constexpr int kExample = app::kBase + 700;

        } // namespace toolbars

        /**
         * @brief Menu identifiers.
         *
         * Used as keys into ui::menus::MenuRegistry (see
         * ui/menus/menu_registry.hpp) - NOT fired as wx command events. A
         * window passes one of these to
         * MenuRegistry::getInstance().Create(id) to instantiate the
         * corresponding wxMenu.
         */
        namespace menus
        {
            constexpr int kExample = app::kBase + 800;

        } // namespace menus

    } // namespace ids
} // namespace ui::framework::wx
