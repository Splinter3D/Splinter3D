#include "key_bind_button.hpp"

#include "ui/framework/wx/locale/locale.hpp"

#include <utility>
#include <wx/accel.h>

using ui::framework::wx::locale::tr;

namespace ui::widgets
{
    namespace
    {
        bool isModifier(int keyCode)
        {
            switch (keyCode)
            {
                case WXK_SHIFT:
                case WXK_ALT:
                case WXK_CONTROL:
                case WXK_RAW_CONTROL:
                case WXK_WINDOWS_LEFT:
                case WXK_WINDOWS_RIGHT:
                case WXK_WINDOWS_MENU:
                    return true;
                default:
                    return false;
            }
        }

        bool parse(const std::string& value, wxAcceleratorEntry& entry)
        {
            return !value.empty() && entry.FromString(wxString::FromUTF8(value));
        }
    } // namespace

    std::string keyBindOf(const wxKeyEvent& event)
    {
        const int keyCode = event.GetKeyCode();
        if (isModifier(keyCode))
            return {};
        int flags = wxACCEL_NORMAL;
        if (event.ControlDown()) // Command on macOS
            flags |= wxACCEL_CTRL;
        if (event.AltDown())
            flags |= wxACCEL_ALT;
        if (event.ShiftDown())
            flags |= wxACCEL_SHIFT;
#if defined(__WXOSX__)
        if (event.RawControlDown()) // the Control key (elsewhere, the same as ControlDown())
            flags |= wxACCEL_RAW_CTRL;
#endif
        return wxAcceleratorEntry(flags, keyCode).ToRawString().utf8_string();
    }

    bool matchesKeyBind(const wxKeyEvent& event, const std::string& value)
    {
        wxAcceleratorEntry entry;
        if (!parse(value, entry))
            return false;
        const std::string pressed = keyBindOf(event);
        return !pressed.empty() && pressed == entry.ToRawString().utf8_string(); // both canonical
    }

    wxString formatKeyBind(const std::string& value)
    {
        wxAcceleratorEntry entry;
        if (!parse(value, entry))
            return {};
#if defined(__WXOSX__)
        const int flags = entry.GetFlags();
        wxString  text;
        if (flags & wxACCEL_RAW_CTRL)
            text += wxString::FromUTF8("\u2303"); // ⌃
        if (flags & wxACCEL_ALT)
            text += wxString::FromUTF8("\u2325"); // ⌥
        if (flags & wxACCEL_SHIFT)
            text += wxString::FromUTF8("\u21E7"); // ⇧
        if (flags & wxACCEL_CTRL)
            text += wxString::FromUTF8("\u2318"); // ⌘
        return text + wxAcceleratorEntry(wxACCEL_NORMAL, entry.GetKeyCode()).ToString();
#else
        return entry.ToString();
#endif
    }

    KeyBindButton::KeyBindButton(wxWindow* parent, const std::string& value, std::function<void(const std::string&)> onChange)
        : wxButton(parent, wxID_ANY)
        , value_(value)
        , on_change_(std::move(onChange))
    {
        showValue();
        Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { StartCapture(); });
        // Before the frame's shortcuts see the keys (see ShortcutEvents).
        Bind(wxEVT_CHAR_HOOK, &KeyBindButton::onKey, this);
        Bind(wxEVT_KILL_FOCUS, [this](wxFocusEvent& event) {
            CancelCapture();
            event.Skip();
        });
    }

    void KeyBindButton::SetValue(const std::string& value)
    {
        value_ = value;
        if (!capturing_)
            showValue();
    }

    std::string KeyBindButton::GetValue() const
    {
        return value_;
    }

    bool KeyBindButton::IsCapturing() const noexcept
    {
        return capturing_;
    }

    void KeyBindButton::StartCapture()
    {
        capturing_ = true;
        SetLabel(tr("settings.keys.press"));
        SetFocus();
    }

    void KeyBindButton::CancelCapture()
    {
        if (!capturing_)
            return;
        capturing_ = false;
        showValue();
    }

    void KeyBindButton::onKey(wxKeyEvent& event)
    {
        if (!capturing_)
        {
            event.Skip();
            return;
        }
        if (event.GetKeyCode() == WXK_ESCAPE)
        {
            CancelCapture();
            return;
        }
        std::string value = keyBindOf(event);
        if (value.empty())
            return; // a modifier alone: wait for the key it goes with

        capturing_ = false;
        value_     = std::move(value);
        showValue();
        if (on_change_)
            on_change_(value_);
    }

    void KeyBindButton::showValue()
    {
        const wxString text = formatKeyBind(value_);
        SetLabel(text.empty() ? tr("settings.keys.none") : text);
    }
} // namespace ui::widgets
