#include "shortcut_events.hpp"

#include "ui/framework/wx/ids/ids.hpp"
#include "ui/widgets/key_bind_button/key_bind_button.hpp"

#include <string>
#include <utility>
#include <wx/accel.h>
#include <wx/menu.h>

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
                int         command;      // menu command sent; 0: model management
            };

            constexpr Shortcut kShortcuts[] = {
                {"keys.new", "Ctrl+N", "settings.keys.new", ids::file::kNew},
                {"keys.open", "Ctrl+O", "settings.keys.open", ids::file::kOpen},
                {"keys.save", "Ctrl+S", "settings.keys.save", ids::file::kSave},
                {"keys.model_management", "Ctrl+Shift+O", "settings.keys.model_management", 0},
#if defined(__WXOSX__)
                // F11 is Mission Control's "Show Desktop" on macOS; ⌃⌘F is the platform's fullscreen key.
                {"keys.fullscreen", "RawCtrl+Ctrl+F", "settings.keys.fullscreen", ids::view::kToggleFullscreen},
#else
                {"keys.fullscreen", "F11", "settings.keys.fullscreen", ids::view::kToggleFullscreen},
#endif
                {"keys.quit", "Ctrl+Q", "settings.keys.quit", ids::file::kExit},
            };

            bool parse(const std::string& value, wxAcceleratorEntry& entry)
            {
                return !value.empty() && entry.FromString(wxString::FromUTF8(value));
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

            const auto& settings = platform::settings::SettingsRegistry::getInstance();
            for (const auto& shortcut : kShortcuts)
            {
                if (!ui::widgets::matchesKeyBind(event, settings.get(shortcut.key)))
                    continue;
                if (shortcut.command == 0)
                {
                    if (on_model_management_)
                        on_model_management_();
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
