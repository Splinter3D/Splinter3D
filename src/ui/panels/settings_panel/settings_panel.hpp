#pragma once

#include "platform/settings/settings_registry.hpp"

#include <functional>
#include <string>
#include <vector>
#include <wx/scrolwin.h>

class wxFlexGridSizer;

namespace ui::widgets
{
    class KeyBindButton;
}

namespace ui::panels
{
    /**
     * @brief Lists every setting declared in the SettingsRegistry, grouped by
     * section, and edits them: a checkbox per Toggle, a drop-down per Choice,
     * a key recorder (plus reset) per KeyBind. Key combinations used twice are
     * flagged.
     *
     * Settings can be declared after it is built (e.g. shortcuts, when the
     * window binds its events): it rebuilds when shown if the list changed.
     */
    class SettingsPanel : public wxScrolledWindow
    {
      public:
        explicit SettingsPanel(wxWindow* parent);

        /** @brief Rebuilds if settings were declared since the last build. */
        void RefreshIfChanged();

      private:
        struct Row
        {
            std::string                             key;
            std::function<void(const std::string&)> show; // reflects a new value
            ui::widgets::KeyBindButton*             keys = nullptr;
        };

        void build();
        /** @brief Adds setting's controls to sizer; keys is the key bind grid being filled, if any. */
        void addSetting(const platform::settings::Setting& setting, wxSizer* sizer, wxFlexGridSizer*& keys);
        void markConflicts();

        std::vector<std::string>                      built_keys_;
        std::vector<Row>                              rows_;
        std::vector<platform::settings::Subscription> subscriptions_;
    };
} // namespace ui::panels
