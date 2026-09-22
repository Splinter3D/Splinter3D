#include "transform_toolbar.hpp"

#include <cmath>
#include <memory>
#include <utility>
#include <wx/dcbuffer.h>
#include <wx/graphics.h>

namespace ui::toolbars
{
    namespace
    {
        constexpr int kIconSize   = 28;
        constexpr int kButtonSide = 64;

        const wxString kAllModelsLabel = "All models";

        // Draws a small filled triangle pointing away from (originX,
        // originY) in the given direction, tip located at (originX,
        // originY) + direction * length.
        void DrawArrowhead(wxGraphicsContext& gc,
                           double             tipX,
                           double             tipY,
                           double             angle,
                           double             length,
                           double             width)
        {
            const double halfWidth = width / 2.0;
            const double backX     = tipX - length * std::cos(angle);
            const double backY     = tipY - length * std::sin(angle);
            const double perpX     = -std::sin(angle);
            const double perpY     = std::cos(angle);

            wxGraphicsPath path = gc.CreatePath();
            path.MoveToPoint(tipX, tipY);
            path.AddLineToPoint(backX + halfWidth * perpX, backY + halfWidth * perpY);
            path.AddLineToPoint(backX - halfWidth * perpX, backY - halfWidth * perpY);
            path.CloseSubpath();
            gc.FillPath(path);
        }

        // Four-way arrow cross, used to represent "move / translate".
        wxBitmap MakeMoveIcon(const wxColour& colour)
        {
            wxBitmap bitmap(kIconSize, kIconSize, 32);
            bitmap.UseAlpha();

            wxMemoryDC dc(bitmap);
            dc.SetBackground(wxBrush(wxColour(0, 0, 0, 0)));
            dc.Clear();

            std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
            if (gc)
            {
                const double centre     = kIconSize / 2.0;
                const double tip        = kIconSize / 2.0 - 1.0;
                const double headLength = 7.0;
                const double headWidth  = 7.0;
                const double shaftStart = 3.5; // gap at the centre, arrows only

                gc->SetPen(wxPen(colour, 3));
                gc->SetBrush(wxBrush(colour));

                // Four independent shafts (not a single cross through the
                // centre) so the middle stays open and reads as a "move"
                // glyph rather than a plain plus sign.
                gc->StrokeLine(centre + shaftStart, centre, centre + tip - headLength, centre);
                gc->StrokeLine(centre - shaftStart, centre, centre - tip + headLength, centre);
                gc->StrokeLine(centre, centre + shaftStart, centre, centre + tip - headLength);
                gc->StrokeLine(centre, centre - shaftStart, centre, centre - tip + headLength);

                DrawArrowhead(*gc, centre + tip, centre, 0.0, headLength, headWidth);
                DrawArrowhead(*gc, centre - tip, centre, M_PI, headLength, headWidth);
                DrawArrowhead(*gc, centre, centre + tip, M_PI_2, headLength, headWidth);
                DrawArrowhead(*gc, centre, centre - tip, -M_PI_2, headLength, headWidth);
            }

            dc.SelectObject(wxNullBitmap);
            return bitmap;
        }

        // Circular arrow (three-quarter ring with an arrowhead), used to
        // represent "rotate".
        wxBitmap MakeRotateIcon(const wxColour& colour)
        {
            wxBitmap bitmap(kIconSize, kIconSize, 32);
            bitmap.UseAlpha();

            wxMemoryDC dc(bitmap);
            dc.SetBackground(wxBrush(wxColour(0, 0, 0, 0)));
            dc.Clear();

            std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
            if (gc)
            {
                const double centre = kIconSize / 2.0;
                const double radius = kIconSize / 2.0 - 5.0;

                // Leaves a clear quarter-circle gap so the shape reads as
                // an open arc, not a closed ring.
                constexpr double kStartAngle = -M_PI_2;
                constexpr double kEndAngle   = M_PI_2 * 3.0 - 0.55;

                gc->SetPen(wxPen(colour, 3));
                gc->SetBrush(wxBrush(colour));

                wxGraphicsPath path = gc->CreatePath();
                path.AddArc(centre, centre, radius, kStartAngle, kEndAngle, true);
                gc->StrokePath(path);

                const double tipAngle = kEndAngle + M_PI_2; // tangent direction
                DrawArrowhead(*gc,
                              centre + radius * std::cos(kEndAngle),
                              centre + radius * std::sin(kEndAngle),
                              tipAngle,
                              8.0,
                              8.0);
            }

            dc.SelectObject(wxNullBitmap);
            return bitmap;
        }
    } // namespace

    TransformToolbar::TransformToolbar(wxWindow* parent)
        : wxPanel(parent, wxID_ANY)
    {
        auto* outerSizer = new wxBoxSizer(wxVERTICAL);
        SetSizer(outerSizer);

        auto* buttonsRow = new wxBoxSizer(wxHORIZONTAL);
        outerSizer->Add(buttonsRow, 0, wxEXPAND);

        const wxColour iconColour = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);

        move_button_ = MakeToggle(MakeMoveIcon(iconColour), "Move");
        buttonsRow->Add(move_button_, 0, wxALIGN_CENTER_VERTICAL | wxALL, 6);

        rotate_button_ = MakeToggle(MakeRotateIcon(iconColour), "Rotate");
        buttonsRow->Add(rotate_button_, 0, wxALIGN_CENTER_VERTICAL | wxALL, 6);

        move_panel_ = MakeFieldsPanel("mm", -100000.0, 100000.0, 0.5, 2,
                                     move_target_combo_, move_x_, move_y_, move_z_);
        outerSizer->Add(move_panel_, 0, wxEXPAND);

        rotate_panel_ = MakeFieldsPanel("°", -360.0, 360.0, 1.0, 1,
                                       rotate_target_combo_, rotate_x_, rotate_y_, rotate_z_);
        outerSizer->Add(rotate_panel_, 0, wxEXPAND);

        move_panel_->Hide();
        rotate_panel_->Hide();

        move_target_combo_->Bind(wxEVT_COMBOBOX, [this](wxCommandEvent&) {
            OnTargetComboChanged(move_target_combo_);
        });
        rotate_target_combo_->Bind(wxEVT_COMBOBOX, [this](wxCommandEvent&) {
            OnTargetComboChanged(rotate_target_combo_);
        });

        // The two panels are mutually exclusive so "the active tool" is
        // always well-defined for the gizmo.
        move_button_->Bind(wxEVT_TOGGLEBUTTON, [this](wxCommandEvent&) {
            if (move_button_->GetValue())
            {
                rotate_button_->SetValue(false);
                ShowFieldsPanel(move_panel_);
            }
            else
                HideFieldsPanels();
            NotifyActiveToolChanged();
        });

        rotate_button_->Bind(wxEVT_TOGGLEBUTTON, [this](wxCommandEvent&) {
            if (rotate_button_->GetValue())
            {
                move_button_->SetValue(false);
                ShowFieldsPanel(rotate_panel_);
            }
            else
                HideFieldsPanels();
            NotifyActiveToolChanged();
        });

        SetEnabled(false);
    }

    void TransformToolbar::SetEnabled(bool enabled)
    {
        if (!enabled)
        {
            move_button_->SetValue(false);
            rotate_button_->SetValue(false);
            HideFieldsPanels();
        }

        move_button_->Enable(enabled);
        rotate_button_->Enable(enabled);
        move_target_combo_->Enable(enabled);
        rotate_target_combo_->Enable(enabled);
        NotifyActiveToolChanged();
    }

    void TransformToolbar::ResetValues()
    {
        move_x_->SetValue(0.0);
        move_y_->SetValue(0.0);
        move_z_->SetValue(0.0);
        rotate_x_->SetValue(0.0);
        rotate_y_->SetValue(0.0);
        rotate_z_->SetValue(0.0);
    }

    void TransformToolbar::SetTargets(const wxArrayString& modelNames)
    {
        const wxString previous = move_target_combo_->GetStringSelection();

        for (wxComboBox* combo : {move_target_combo_, rotate_target_combo_})
        {
            combo->Clear();
            combo->Append(kAllModelsLabel);
            combo->Append(modelNames);

            const int previousIndex = combo->FindString(previous);
            combo->SetSelection(previousIndex != wxNOT_FOUND ? previousIndex : 0);
        }
    }

    int TransformToolbar::GetTargetIndex() const
    {
        const int selection = move_target_combo_->GetSelection();
        return selection <= 0 ? -1 : selection - 1;
    }

    void TransformToolbar::SetOnTargetChanged(std::function<void(int)> callback)
    {
        on_target_changed_ = std::move(callback);
    }

    TransformToolbar::ActiveTool TransformToolbar::GetActiveTool() const
    {
        if (move_button_->GetValue())
            return ActiveTool::Move;
        if (rotate_button_->GetValue())
            return ActiveTool::Rotate;
        return ActiveTool::None;
    }

    void TransformToolbar::SetOnActiveToolChanged(std::function<void(ActiveTool)> callback)
    {
        on_active_tool_changed_ = std::move(callback);
    }

    void TransformToolbar::NotifyActiveToolChanged()
    {
        if (on_active_tool_changed_)
            on_active_tool_changed_(GetActiveTool());
    }

    void TransformToolbar::OnTargetComboChanged(wxComboBox* source)
    {
        wxComboBox* other = source == move_target_combo_ ? rotate_target_combo_ : move_target_combo_;
        other->SetSelection(source->GetSelection());

        if (on_target_changed_)
            on_target_changed_(GetTargetIndex());
    }

    wxToggleButton* TransformToolbar::MakeToggle(const wxBitmap& icon, const wxString& label)
    {
        auto* button = new wxToggleButton(this, wxID_ANY, label, wxDefaultPosition,
                                          wxSize(kButtonSide, kButtonSide));
        button->SetBitmap(icon, wxTOP);
        return button;
    }

    wxPanel* TransformToolbar::MakeFieldsPanel(const wxString&    unit,
                                               double             min_range,
                                               double             max_range,
                                               double             increment,
                                               int                digits,
                                               wxComboBox*&       outTarget,
                                               wxSpinCtrlDouble*& outX,
                                               wxSpinCtrlDouble*& outY,
                                               wxSpinCtrlDouble*& outZ)
    {
        auto* panel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                  wxBORDER_SIMPLE);

        outTarget = new wxComboBox(panel, wxID_ANY, kAllModelsLabel, wxDefaultPosition,
                                   wxSize(160, -1), 0, nullptr, wxCB_READONLY);
        outTarget->Append(kAllModelsLabel);
        outTarget->SetSelection(0);

        auto*        fieldsSizer = new wxBoxSizer(wxHORIZONTAL);
        const wxSize spinSize(80, 28);

        auto addAxis = [&](const wxString& axisLabel, wxSpinCtrlDouble*& out) {
            fieldsSizer->Add(new wxStaticText(panel, wxID_ANY, axisLabel), 0,
                             wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);

            out = new wxSpinCtrlDouble(panel, wxID_ANY, wxString::Format("%.*f", digits, 0.0),
                                       wxDefaultPosition, spinSize, wxSP_ARROW_KEYS,
                                       min_range, max_range, 0.0, increment);
            out->SetDigits(static_cast<unsigned int>(digits));
            fieldsSizer->Add(out, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
        };

        addAxis("X", outX);
        addAxis("Y", outY);
        addAxis("Z", outZ);

        fieldsSizer->Add(new wxStaticText(panel, wxID_ANY, unit), 0, wxALIGN_CENTER_VERTICAL);

        auto* outerSizer = new wxBoxSizer(wxVERTICAL);
        outerSizer->Add(outTarget, 0, wxLEFT | wxRIGHT | wxTOP | wxEXPAND, 10);
        outerSizer->Add(fieldsSizer, 0, wxALL, 10);
        panel->SetSizer(outerSizer);

        return panel;
    }

    void TransformToolbar::ShowFieldsPanel(wxPanel* panel)
    {
        move_panel_->Show(panel == move_panel_);
        rotate_panel_->Show(panel == rotate_panel_);
        Layout();
        if (GetParent() != nullptr)
            GetParent()->Layout();
    }

    void TransformToolbar::HideFieldsPanels()
    {
        move_panel_->Hide();
        rotate_panel_->Hide();
        Layout();
        if (GetParent() != nullptr)
            GetParent()->Layout();
    }

} // namespace ui::toolbars
