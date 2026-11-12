#include "settings_panel.hpp"

#include "ui/framework/wx/locale/locale.hpp"
#include "ui/widgets/key_bind_button/key_bind_button.hpp"

#include <algorithm>
#include <map>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

using ui::framework::wx::locale::tr;

namespace ui::panels
{
    namespace
    {
        constexpr int kWidth = 280;

        using platform::settings::Kind;
        using platform::settings::Setting;
        using platform::settings::SettingsRegistry;

        std::vector<std::string> declaredKeys()
        {
            std::vector<std::string> keys;
            for (const auto& setting : SettingsRegistry::getInstance().settings())
                keys.push_back(setting.key);
            return keys;
        }
    } // namespace

    SettingsPanel::SettingsPanel(wxWindow* parent)
        : wxScrolledWindow(parent, wxID_ANY)
    {
        SetMinSize(wxSize(kWidth, -1));
        SetScrollRate(0, 10);
        build();
        Bind(wxEVT_SHOW, [this](wxShowEvent& event) {
            if (event.IsShown())
                RefreshIfChanged();
            event.Skip();
        });
    }

    void SettingsPanel::RefreshIfChanged()
    {
        if (declaredKeys() != built_keys_)
            build();
    }

    void SettingsPanel::build()
    {
        subscriptions_.clear();
        rows_.clear();
        DestroyChildren();
        built_keys_ = declaredKeys();

        auto* root  = new wxBoxSizer(wxVERTICAL);
        auto* title = new wxStaticText(this, wxID_ANY, tr("settings.title"));
        title->SetFont(title->GetFont().Bold());
        root->Add(title, 0, wxALL, 8);

        // Sections in the order their first setting was declared.
        const auto&              settings = SettingsRegistry::getInstance().settings();
        std::vector<std::string> sections;
        for (const auto& setting : settings)
            if (std::find(sections.begin(), sections.end(), setting.section) == sections.end())
                sections.push_back(setting.section);

        for (const auto& section : sections)
        {
            auto* heading = new wxStaticText(this, wxID_ANY, tr(section));
            heading->SetFont(heading->GetFont().Bold());
            root->Add(heading, 0, wxLEFT | wxRIGHT | wxTOP, 8);
            auto* list = new wxBoxSizer(wxVERTICAL);
            for (const auto& setting : settings)
                if (setting.section == section)
                    addSetting(setting, list);
            root->Add(list, 0, wxEXPAND | wxLEFT | wxRIGHT, 8);
        }

        SetSizer(root); // deletes the previous one
        FitInside();
        markConflicts();
        Layout();
    }

    void SettingsPanel::addSetting(const Setting& setting, wxSizer* sizer)
    {
        auto&             settings = SettingsRegistry::getInstance();
        const std::string key      = setting.key;
        const std::string value    = settings.get(key);
        Row               row;
        row.key = key;

        switch (setting.kind)
        {
            case Kind::Toggle: {
                auto* box = new wxCheckBox(this, wxID_ANY, tr(setting.label));
                box->SetValue(value == "1");
                box->Bind(wxEVT_CHECKBOX, [key](wxCommandEvent& event) {
                    SettingsRegistry::getInstance().set(key, event.IsChecked() ? "1" : "0");
                });
                row.show = [box](const std::string& v) { box->SetValue(v == "1"); };
                sizer->Add(box, 0, wxTOP, 6);
                break;
            }
            case Kind::Choice: {
                sizer->Add(new wxStaticText(this, wxID_ANY, tr(setting.label)), 0, wxTOP, 6);
                auto* choice = new wxChoice(this, wxID_ANY);
                for (const auto& option : setting.choices)
                    choice->Append(tr(option.label));
                const auto selectionOf = [choices = setting.choices](const std::string& v) {
                    const auto found = std::find_if(choices.begin(), choices.end(), [&](const auto& c) { return c.value == v; });
                    return found != choices.end() ? static_cast<int>(found - choices.begin()) : wxNOT_FOUND;
                };
                choice->SetSelection(selectionOf(value));
                choice->Bind(wxEVT_CHOICE, [key, choices = setting.choices](wxCommandEvent& event) {
                    const int selection = event.GetSelection();
                    if (selection >= 0 && static_cast<std::size_t>(selection) < choices.size())
                        SettingsRegistry::getInstance().set(key, choices[static_cast<std::size_t>(selection)].value);
                });
                row.show = [choice, selectionOf](const std::string& v) { choice->SetSelection(selectionOf(v)); };
                sizer->Add(choice, 0, wxEXPAND | wxTOP, 2);
                break;
            }
            case Kind::KeyBind: {
                auto* line  = new wxBoxSizer(wxHORIZONTAL);
                auto* label = new wxStaticText(this, wxID_ANY, tr(setting.label));
                row.keys    = new ui::widgets::KeyBindButton(this, value, [key](const std::string& v) {
                    SettingsRegistry::getInstance().set(key, v);
                });
                auto* reset = new wxButton(this, wxID_ANY, wxString::FromUTF8("\u21BA"), wxDefaultPosition, wxDefaultSize,
                                           wxBU_EXACTFIT); // ↺
                reset->SetToolTip(tr("settings.keys.reset_tooltip"));
                reset->Bind(wxEVT_BUTTON, [key](wxCommandEvent&) { SettingsRegistry::getInstance().reset(key); });
                row.show = [keys = row.keys](const std::string& v) { keys->SetValue(v); };
                line->Add(label, 1, wxALIGN_CENTER_VERTICAL);
                line->Add(row.keys, 0, wxLEFT, 6);
                line->Add(reset, 0, wxLEFT, 2);
                sizer->Add(line, 0, wxEXPAND | wxTOP, 4);
                break;
            }
            default:
                return;
        }

        if (!setting.note.empty())
        {
            auto* note = new wxStaticText(this, wxID_ANY, tr(setting.note));
            note->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));
            note->Wrap(kWidth - 24);
            sizer->Add(note, 0, wxTOP, 2);
        }

        subscriptions_.push_back(settings.subscribe(key, [this, show = row.show](const std::string& v) {
            show(v);
            markConflicts();
        }));
        rows_.push_back(std::move(row));
    }

    void SettingsPanel::markConflicts()
    {
        std::map<std::string, int> uses;
        for (const auto& row : rows_)
            if (row.keys != nullptr && !row.keys->GetValue().empty())
                ++uses[row.keys->GetValue()];
        for (const auto& row : rows_)
        {
            if (row.keys == nullptr)
                continue;
            const bool conflict = !row.keys->GetValue().empty() && uses[row.keys->GetValue()] > 1;
            row.keys->SetForegroundColour(conflict ? wxColour(200, 40, 40) : wxNullColour);
            row.keys->SetToolTip(conflict ? tr("settings.keys.conflict") : wxString());
            row.keys->Refresh();
        }
    }
} // namespace ui::panels
