#include "shortcut_events.hpp"

#include "ui/framework/wx/ids/ids.hpp"
#include "ui/widgets/key_bind_button/key_bind_button.hpp"

#include <string>
#include <utility>
#include <wx/accel.h>
#include <wx/combobox.h>
#include <wx/menu.h>
#include <wx/textentry.h>

namespace ui::framework::wx
{
    namespace events
    {
        namespace
        {
            struct Shortcut
            {
                const char* key;          // setting key
                const char* defaultValue; // wxAcceleratorEntry raw string
                const char* label;        // locale key
                int         command;      // menu command sent; 0: its action (see ShortcutEvents::actions_)
            };

            constexpr Shortcut kShortcuts[] = {
                {"keys.new", "Ctrl+N", "settings.keys.new", ids::file::kNew},
                {"keys.open", "Ctrl+O", "settings.keys.open", ids::file::kOpen},
                {"keys.save", "Ctrl+S", "settings.keys.save", ids::file::kSave},
                {"keys.model_management", "Ctrl+Shift+O", "settings.keys.model_management", 0},
                {"keys.tasks_panel", "Ctrl+Shift+T", "settings.keys.tasks_panel", 0},
                {"keys.settings_panel", "Ctrl+,", "settings.keys.settings_panel", 0},
#if defined(__WXOSX__)
                // F11 is Mission Control's "Show Desktop" on macOS; ⌃⌘F is the platform's fullscreen key.
                {"keys.fullscreen", "RawCtrl+Ctrl+F", "settings.keys.fullscreen", ids::view::kToggleFullscreen},
#else
                {"keys.fullscreen", "F11", "settings.keys.fullscreen", ids::view::kToggleFullscreen},
#endif
                {"keys.quit", "Ctrl+Q", "settings.keys.quit", ids::file::kExit},
#if defined(__WXOSX__)
                {"keys.delete_model", "Back", "settings.keys.delete_model", 0},
#else
                {"keys.delete_model", "Delete", "settings.keys.delete_model", 0},
#endif
                {"keys.hide_selection", "H", "settings.keys.hide_selection", 0},
                {"keys.duplicate", "Ctrl+D", "settings.keys.duplicate", 0},
                {"keys.deselect", "Escape", "settings.keys.deselect", 0},
            };

            bool parse(const std::string& value, wxAcceleratorEntry& entry)
            {
                return !value.empty() && entry.FromString(wxString::FromUTF8(value));
            }

            // Whether a text field with the focus should get this key rather
            // than the shortcuts: no Ctrl/Command, Control or Alt, and neither
            // a function key nor Escape (which edits nothing).
            bool editsText(const wxKeyEvent& event)
            {
                wxWindow* focus = wxWindow::FindFocus();
                if (dynamic_cast<wxTextEntry*>(focus) == nullptr)
                    return false;
                // A read-only combo box (the transform target) is a text entry
                // nobody types in.
                if (const auto* combo = dynamic_cast<wxComboBox*>(focus); combo != nullptr && combo->HasFlag(wxCB_READONLY))
                    return false;
                if (event.ControlDown() || event.RawControlDown() || event.AltDown())
                    return false;
                const int key = event.GetKeyCode();
                return key != WXK_ESCAPE && (key < WXK_F1 || key > WXK_F24);
            }
        } // namespace

        void ShortcutEvents::bind()
        {
            using platform::settings::Kind;
            auto& settings = platform::settings::SettingsRegistry::getInstance();
            for (const auto& shortcut : kShortcuts)
                subscriptions_.push_back(settings.add(
                    {shortcut.key, "settings.section.keys", shortcut.label, "", Kind::KeyBind, shortcut.defaultValue, {}},
                    [this](const std::string&) { syncMenus(); }));

            frame_->Bind(wxEVT_CHAR_HOOK, &ShortcutEvents::onKey, this);
            syncMenus();
        }

        void ShortcutEvents::onKey(wxKeyEvent& event)
        {
            // A key being recorded in the settings is not a shortcut.
            if (const auto* recorder = dynamic_cast<ui::widgets::KeyBindButton*>(wxWindow::FindFocus());
                recorder != nullptr && recorder->IsCapturing())
            {
                event.Skip();
                return;
            }
            if (editsText(event))
            {
                event.Skip();
                return;
            }

            const auto& settings = platform::settings::SettingsRegistry::getInstance();
            for (const auto& shortcut : kShortcuts)
            {
                if (!ui::widgets::matchesKeyBind(event, settings.get(shortcut.key)))
                    continue;
                if (shortcut.command == 0)
                {
                    const auto action = actions_.find(shortcut.key);
                    if (action == actions_.end() || !action->second || !action->second())
                        event.Skip();
                    return;
                }
                wxCommandEvent command(wxEVT_MENU, shortcut.command);
                frame_->ProcessWindowEvent(command);
                return;
            }
            event.Skip();
        }

        void ShortcutEvents::syncMenus()
        {
            wxMenuBar* menuBar = frame_->GetMenuBar();
            if (menuBar == nullptr)
                return;
            const auto& settings = platform::settings::SettingsRegistry::getInstance();
            for (const auto& shortcut : kShortcuts)
            {
                wxMenuItem* item = shortcut.command != 0 ? menuBar->FindItem(shortcut.command) : nullptr;
                if (item == nullptr)
                    continue;
                wxString           label = item->GetItemLabel().BeforeFirst('\t');
                wxAcceleratorEntry entry;
                if (parse(settings.get(shortcut.key), entry))
                    label += "\t" + entry.ToRawString();
                item->SetItemLabel(label);
            }
        }

    } // namespace events
} // namespace ui::framework::wx
