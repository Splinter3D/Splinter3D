#pragma once

#include <functional>
#include <string>
#include <wx/button.h>

class wxKeyEvent;

namespace ui::widgets
{
    /*
     * Key combinations are stored as wxAcceleratorEntry::ToRawString() writes
     * them. "Ctrl" is the primary modifier: Control on Windows and Linux, but
     * Command on macOS, where "RawCtrl" is the Control key. So the defaults
     * ("Ctrl+S") follow each platform's convention.
     */

    /** @brief The combination a key press is, as stored; empty for a lone modifier. */
    [[nodiscard]] std::string keyBindOf(const wxKeyEvent& event);

    /** @brief Whether a key press is the stored combination `value`. */
    [[nodiscard]] bool matchesKeyBind(const wxKeyEvent& event, const std::string& value);

    /**
     * @brief How a stored combination reads: "⌃⌥⇧⌘S" on macOS (Control,
     * Option, Shift, Command, in Apple's order), "Ctrl+Alt+Shift+S" elsewhere;
     * empty if it is none or can't be read.
     */
    [[nodiscard]] wxString formatKeyBind(const std::string& value);

    /**
     * @brief A button showing a key combination; clicked, it records the next
     * one pressed (Escape or losing focus cancels).
     *
     * Combinations are strings as keyBindOf() makes them ("Ctrl+Shift+O"),
     * shown with formatKeyBind(); empty means none.
     */
    class KeyBindButton : public wxButton
    {
      public:
        KeyBindButton(wxWindow* parent, const std::string& value, std::function<void(const std::string&)> onChange);

        void                      SetValue(const std::string& value);
        [[nodiscard]] std::string GetValue() const;

        /** @brief Whether it is waiting for a combination (clicked, nothing pressed yet). */
        [[nodiscard]] bool IsCapturing() const noexcept;

        void StartCapture();
        void CancelCapture();

      private:
        void onKey(wxKeyEvent& event);
        void showValue();

        std::string                             value_;
        bool                                    capturing_ = false;
        std::function<void(const std::string&)> on_change_;
    };
} // namespace ui::widgets
