#pragma once

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

      private:
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
        std::unique_ptr<Viewer> viewer_;
        bool                    initialized_ = false;
        bool                    rotating_    = false;
        bool                    panning_     = false;
        wxPoint                 last_mouse_position_;
    };
} // namespace ui::framework::occt
