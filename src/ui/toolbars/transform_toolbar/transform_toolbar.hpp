#pragma once

#include <functional>
#include <wx/combobox.h>
#include <wx/spinctrl.h>
#include <wx/tglbtn.h>
#include <wx/wx.h>

namespace ui::toolbars
{
    /**
     * @brief Bar exposing the position (mm) and rotation (degrees) of the
     * currently loaded model, under two square toggle buttons ("Move" and
     * "Rotate"). Clicking a button reveals its X/Y/Z fields directly below
     * the buttons, inline in the toolbar's own layout; clicking it again
     * (or the other button) hides them.
     *
     * The fields are a plain, embedded panel rather than a floating popup
     * on purpose: a popup would grab the mouse while open, which prevents
     * clicking anywhere else - including the 3D viewport - while the user
     * still needs to interact with it (e.g. to drag a gizmo or pick a
     * model). Embedding the fields means showing them only changes the
     * toolbar's own height; it never blocks input to the rest of the
     * window.
     *
     * Both buttons start disabled: callers should enable them (SetEnabled)
     * once a model is loaded, and disable them again once it isn't. This
     * bar has no idea whether a model is loaded - that decision belongs to
     * whoever owns both the model viewer and this toolbar.
     */
    class TransformToolbar : public wxPanel
    {
      public:
        /**
         * @brief Constructs the transform bar. Both buttons start disabled.
         *
         * @param parent Parent window that owns the bar.
         */
        explicit TransformToolbar(wxWindow* parent);

        /**
         * @brief Enables or disables both buttons.
         *
         * Disabling also hides the fields panel and resets both buttons to
         * their unpressed state.
         */
        void SetEnabled(bool enabled);

        /**
         * @brief Resets all six fields back to 0.
         */
        void ResetValues();

        /**
         * @brief Sets the list of loaded model names shown in the target
         * dropdown of both panels. "All models" is always prepended and
         * kept selected if it already was, or if the previously selected
         * name is no longer present in `modelNames`.
         */
        void SetTargets(const wxArrayString& modelNames);

        /**
         * @brief Currently selected target (shared by both panels): -1
         * means "All models", otherwise a 0-based index into the names
         * passed to the last SetTargets() call.
         */
        [[nodiscard]] int GetTargetIndex() const;

        /**
         * @brief Invoked whenever the user picks a different target in
         * either panel's dropdown, with the same value GetTargetIndex()
         * would then return.
         */
        void SetOnTargetChanged(std::function<void(int)> callback);

        /**
         * @brief Which tool's fields are currently shown, if any. The two
         * panels are mutually exclusive: showing one hides the other.
         */
        enum class ActiveTool
        {
            None,
            Move,
            Rotate
        };

        [[nodiscard]] ActiveTool GetActiveTool() const;

        /**
         * @brief Invoked whenever the active tool changes (a panel is
         * shown, hidden, or the bar gets disabled), with the same value
         * GetActiveTool() would then return.
         */
        void SetOnActiveToolChanged(std::function<void(ActiveTool)> callback);

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
        // Creates the square icon-on-top toggle button used to show/hide a
        // group's fields panel ("Move" / "Rotate").
        wxToggleButton* MakeToggle(const wxBitmap& icon, const wxString& label);

        // Builds the embedded panel with the "target" dropdown and the
        // "X/Y/Z fields + unit" for one tool. outTarget receives the
        // created target combo, outX/outY/outZ the created spin controls.
        // The panel is added to `fields_sizer_`, initially hidden.
        wxPanel* MakeFieldsPanel(const wxString&    unit,
                                 double             min_range,
                                 double             max_range,
                                 double             increment,
                                 int                digits,
                                 wxComboBox*&       outTarget,
                                 wxSpinCtrlDouble*& outX,
                                 wxSpinCtrlDouble*& outY,
                                 wxSpinCtrlDouble*& outZ);

        // Keeps both panels' target dropdowns in sync with `source`, then
        // notifies on_target_changed_.
        void OnTargetComboChanged(wxComboBox* source);

        // Shows `panel` (hiding the other one) and re-layouts the toolbar
        // and its parent so the new height takes effect immediately.
        void ShowFieldsPanel(wxPanel* panel);

        // Hides both panels and re-layouts, without touching button state.
        void HideFieldsPanels();

        // Notifies on_active_tool_changed_ with GetActiveTool().
        void NotifyActiveToolChanged();

        wxComboBox*                     move_target_combo_   = nullptr;
        wxComboBox*                     rotate_target_combo_ = nullptr;
        std::function<void(int)>        on_target_changed_;
        std::function<void(ActiveTool)> on_active_tool_changed_;

        wxToggleButton* move_button_   = nullptr;
        wxToggleButton* rotate_button_ = nullptr;

        wxPanel* move_panel_   = nullptr;
        wxPanel* rotate_panel_ = nullptr;

        wxSpinCtrlDouble* move_x_   = nullptr;
        wxSpinCtrlDouble* move_y_   = nullptr;
        wxSpinCtrlDouble* move_z_   = nullptr;
        wxSpinCtrlDouble* rotate_x_ = nullptr;
        wxSpinCtrlDouble* rotate_y_ = nullptr;
        wxSpinCtrlDouble* rotate_z_ = nullptr;
    };

} // namespace ui::toolbars
