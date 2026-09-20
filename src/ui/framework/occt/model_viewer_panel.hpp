#pragma once

#include <functional>
#include <memory>
#include <wx/glcanvas.h>
#include <wx/panel.h>

namespace ui::framework::occt
{
    class ModelViewerPanel final : public wxPanel
    {
      public:
        explicit ModelViewerPanel(wxWindow* parent);
        ~ModelViewerPanel() override;

        void newFile();
        void openFile();
        void saveFile();

        // True as soon as a shape is loaded in the viewer.
        [[nodiscard]] bool hasModel() const;

        // Moves (millimeters) and rotates (degrees, around the origin) the
        // currently loaded model. Does nothing if no model is loaded.
        void setTransform(double moveXmm,
                          double moveYmm,
                          double moveZmm,
                          double rotateXdeg,
                          double rotateYdeg,
                          double rotateZdeg);

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
        bool                      initialized_ = false;
        bool                      rotating_    = false;
        bool                      panning_     = false;
        wxPoint                   last_mouse_position_;
        std::function<void(bool)> on_model_state_changed_;
    };
} // namespace ui::framework::occt
