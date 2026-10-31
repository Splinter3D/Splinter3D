#pragma once

#include <functional>
#include <memory>
#include <vector>
#include <wx/glcanvas.h>
#include <wx/panel.h>
#include <wx/string.h>

namespace ui::framework::occt
{
    class ModelViewerPanel final : public wxPanel
    {
      public:
        // Move (millimeters) and rotate (degrees, around the origin) values
        // currently applied to a given target. See setTransform().
        struct TransformValues
        {
            double moveX   = 0.0;
            double moveY   = 0.0;
            double moveZ   = 0.0;
            double rotateX = 0.0;
            double rotateY = 0.0;
            double rotateZ = 0.0;
        };

        explicit ModelViewerPanel(wxWindow* parent);
        ~ModelViewerPanel() override;

        void newFile();
        void openFile();
        void saveFile();

        // True as soon as a shape is loaded in the viewer.
        [[nodiscard]] bool hasModel() const;

        // Display names of every currently loaded model, in load order.
        // Index i matches setTransform()/GetTransform()'s targetIndex.
        [[nodiscard]] std::vector<wxString> GetModelNames() const;

        [[nodiscard]] std::vector<bool> GetModelVisibility() const;
        void                            SetModelVisibility(int modelIndex, bool visible);
        void                            RemoveModel(int modelIndex);
        void                            FocusModel(int modelIndex);

        // Moves (millimeters) and rotates (degrees, around the origin) a
        // model, replacing whatever transform was previously applied to it.
        // targetIndex selects which one: -1 applies the same absolute
        // transform to every loaded model, otherwise it is an index into
        // GetModelNames(). Does nothing if no model is loaded or the index
        // is out of range.
        void setTransform(int    targetIndex,
                          double moveXmm,
                          double moveYmm,
                          double moveZmm,
                          double rotateXdeg,
                          double rotateYdeg,
                          double rotateZdeg);

        // Returns the move/rotate values currently applied to a target
        // (same indexing as setTransform). All-zero if targetIndex is -1
        // or out of range.
        [[nodiscard]] TransformValues GetTransform(int targetIndex) const;

        // Which interactive 3D gizmo (if any) should be shown in the
        // viewport, mirroring the toolbar's active tool.
        enum class GizmoTool
        {
            None,
            Move,
            Rotate
        };

        // Shows/hides/repositions the interactive gizmo. `targetIndex` uses
        // the same indexing as setTransform(), but -1 ("all models") always
        // hides the gizmo since there is no single meaningful pivot for it.
        void SetGizmoState(int targetIndex, GizmoTool tool);

        // Invoked whenever a gizmo drag changes a model's transform, so
        // callers (e.g. the numeric fields) can refresh from GetTransform().
        void SetOnGizmoChanged(std::function<void()> callback);

        // Invoked with hasModel() every time a model is loaded or cleared,
        // so other widgets (e.g. the transform toolbar) can react.
        void SetOnModelStateChanged(std::function<void(bool)> callback);

      private:
        void notifyModelStateChanged();

        void onPaint(wxPaintEvent& event);
        void onSize(wxSizeEvent& event);
        void onMouseDown(wxMouseEvent& event);
        void onMouseUp(wxMouseEvent& event);
        void onMouseMove(wxMouseEvent& event);
        void onMouseWheel(wxMouseEvent& event);
        void initializeViewer();

        std::unique_ptr<wxGLContext> context_;
        wxGLCanvas*                  canvas_ = nullptr;
        class Viewer;
        std::unique_ptr<Viewer>   viewer_;
        bool                      initialized_    = false;
        bool                      rotating_       = false;
        bool                      panning_        = false;
        bool                      gizmo_dragging_ = false;
        wxPoint                   last_mouse_position_;
        std::function<void(bool)> on_model_state_changed_;
        std::function<void()>     on_gizmo_changed_;
    };
} // namespace ui::framework::occt
