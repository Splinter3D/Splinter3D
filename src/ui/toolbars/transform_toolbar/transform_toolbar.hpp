#pragma once

#include <wx/popupwin.h>
#include <wx/spinctrl.h>
#include <wx/tglbtn.h>
#include <wx/wx.h>

namespace ui::toolbars
{
    /**
     * @brief Bar exposing the position (mm) and rotation (degrees) of the
     * currently loaded model, under two square toggle buttons ("Move" and
     * "Rotate"). Clicking a button drops down a small popup with its X/Y/Z
     * fields right below it; clicking it again (or clicking elsewhere)
     * closes the popup.
     *
     * The bar itself only ever occupies the width of its buttons, so more
     * buttons can be added to its right without their position depending on
     * whether a popup happens to be open.
     *
     * It is purely a UI surface for now: the spin controls it exposes are
     * not yet wired to the model viewer. Callers can reach them through the
     * accessors below once that wiring is implemented.
     */
    class TransformToolbar : public wxPanel
    {
      public:
        /**
         * @brief Constructs the transform bar.
         *
         * @param parent Parent window that owns the bar.
         */
        explicit TransformToolbar(wxWindow* parent);

        // Position spin controls, in millimeters.
        [[nodiscard]] wxSpinCtrlDouble* GetMoveX() const
        {
            return move_x_;
        }

        [[nodiscard]] wxSpinCtrlDouble* GetMoveY() const
        {
            return move_y_;
        }

        [[nodiscard]] wxSpinCtrlDouble* GetMoveZ() const
        {
            return move_z_;
        }

        // Rotation spin controls, in degrees.
        [[nodiscard]] wxSpinCtrlDouble* GetRotateX() const
        {
            return rotate_x_;
        }

        [[nodiscard]] wxSpinCtrlDouble* GetRotateY() const
        {
            return rotate_y_;
        }

        [[nodiscard]] wxSpinCtrlDouble* GetRotateZ() const
        {
            return rotate_z_;
        }

      private:
        // Creates the square icon-on-top toggle button used to open a
        // group's popup ("Move" / "Rotate").
        wxToggleButton* MakeToggle(const wxBitmap& icon, const wxString& label);

        // Builds the popup with the "X/Y/Z fields + unit" revealed below
        // the corresponding toggle button. outX/outY/outZ receive the
        // created spin controls. The popup resets `owner`'s toggle state
        // whenever it gets dismissed (e.g. by clicking elsewhere).
        wxPopupTransientWindow* MakePopup(wxToggleButton*    owner,
                                          const wxString&    unit,
                                          double             min_range,
                                          double             max_range,
                                          double             increment,
                                          int                digits,
                                          wxSpinCtrlDouble*& outX,
                                          wxSpinCtrlDouble*& outY,
                                          wxSpinCtrlDouble*& outZ);

        // Opens `popup` right below `button`, sized to fit its content.
        static void ShowPopupBelow(wxPopupTransientWindow* popup, wxWindow* button);

        wxToggleButton* move_button_   = nullptr;
        wxToggleButton* rotate_button_ = nullptr;

        wxPopupTransientWindow* move_popup_   = nullptr;
        wxPopupTransientWindow* rotate_popup_ = nullptr;

        wxSpinCtrlDouble* move_x_   = nullptr;
        wxSpinCtrlDouble* move_y_   = nullptr;
        wxSpinCtrlDouble* move_z_   = nullptr;
        wxSpinCtrlDouble* rotate_x_ = nullptr;
        wxSpinCtrlDouble* rotate_y_ = nullptr;
        wxSpinCtrlDouble* rotate_z_ = nullptr;
    };

} // namespace ui::toolbars
