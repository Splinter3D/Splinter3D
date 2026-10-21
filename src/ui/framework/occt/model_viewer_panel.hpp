#pragma once

#include "application/service/scene_service.hpp"
#include "geometry/occt/transform.hpp"

#include <filesystem>
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
        using TransformValues = geometry::occt::Transform;

        explicit ModelViewerPanel(wxWindow* parent);
        ~ModelViewerPanel() override;

        void newFile();
        // Asks for an STL file with a dialog, then loadFile()s it.
        void openFile();
        // Asks for a destination with a dialog, then exportFile()s to it.
        void saveFile();

        // Imports an STL file into the scene and displays it. Logs and
        // returns false if it could not be imported (the scene is unchanged).
        bool loadFile(const std::filesystem::path& path);

        // Writes the scene to an STL file. Logs and returns false on failure.
        bool exportFile(const std::filesystem::path& path);

        // True as soon as a shape is loaded in the viewer.
        [[nodiscard]] bool hasModel() const;

        // Display names of every currently loaded model, in load order.
        // Index i matches setTransform()/GetTransform()'s targetIndex.
        [[nodiscard]] std::vector<wxString> GetModelNames() const;

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

        // Declared before viewer_, which holds a reference to it.
        application::service::SceneService scene_;
        std::unique_ptr<wxGLContext>       context_;
        wxGLCanvas*                        canvas_ = nullptr;
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
