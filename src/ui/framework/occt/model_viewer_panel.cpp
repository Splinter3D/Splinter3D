#include "model_viewer_panel.hpp"

#include "geometry/occt/transform.hpp"
#include "ui/dialogs/dialogs.hpp"

#include <AIS_DisplayMode.hxx>
#include <AIS_InteractiveContext.hxx>
#include <AIS_Manipulator.hxx>
#include <AIS_ManipulatorMode.hxx>
#include <AIS_Shape.hxx>
#include <Aspect_DisplayConnection.hxx>
#include <BRep_Builder.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <Precision.hxx>
#include <TopoDS_Vertex.hxx>
#if defined(__APPLE__)
#include <Cocoa_Window.hxx>
#elif defined(_WIN32)
#include <WNT_Window.hxx>
#else
#include <Xw_Window.hxx>
#endif
#include <Quantity_Color.hxx>
#include <TopLoc_Location.hxx>
#include <V3d_AmbientLight.hxx>
#include <V3d_DirectionalLight.hxx>
#include <V3d_View.hxx>
#include <V3d_Viewer.hxx>
#include <algorithm>
#include <filesystem>
#include <gp_Ax2.hxx>
#include <gp_Pnt.hxx>
#include <gp_Trsf.hxx>
#include <vector>
#include <wx/dcclient.h>
#include <wx/log.h>
#include <wx/sizer.h>
#include <wx/stdpaths.h>
#if defined(__WXGTK__)
#include <wx/gtk/glcanvas.h>
#endif

namespace ui::framework::occt
{
    namespace
    {
        // wxString -> path without going through the narrow locale encoding
        // (ToStdString()), which loses non-ASCII characters on Windows.
        std::filesystem::path toPath(const wxString& value)
        {
            const std::string utf8 = value.utf8_string();
            return std::filesystem::path(std::u8string(utf8.begin(), utf8.end()));
        }
    } // namespace

    class ModelViewerPanel::Viewer
    {
      public:
        Viewer(wxGLCanvas* canvas, application::service::SceneService& scene)
            : scene_(scene)
        {
            auto displayConnection = new Aspect_DisplayConnection();
            auto graphicDriver     = new OpenGl_GraphicDriver(displayConnection);
            viewer_                = new V3d_Viewer(graphicDriver);

            // A headlight (theIsHeadlight = true) always shines from the
            // camera towards whatever it is looking at, so orbiting the
            // camera never turns a face away from the light - the default
            // fixed, world-space light (SetDefaultLights()) left faces
            // dark whenever they turned away from it, which looked like
            // flickering when rotating a shape with only a handful of
            // flat faces (e.g. a cube). The ambient light keeps faces the
            // headlight doesn't directly face from going fully black.
            occ::handle<V3d_DirectionalLight> headlight =
                new V3d_DirectionalLight(V3d_Zneg, Quantity_NOC_WHITE, true);
            viewer_->AddLight(headlight);
            viewer_->AddLight(new V3d_AmbientLight(Quantity_NOC_WHITE));
            viewer_->SetLightOn();

            view_ = viewer_->CreateView();
#if defined(__APPLE__)
            auto nativeWindow = new Cocoa_Window(
                reinterpret_cast<NSView*>(canvas->GetHandle()));
#elif defined(_WIN32)
            auto nativeWindow = new WNT_Window(
                reinterpret_cast<Aspect_Handle>(canvas->GetHandle()));
#elif defined(wxUSE_GLCANVAS_EGL) && wxUSE_GLCANVAS_EGL
            auto nativeWindow = new Xw_Window(
                displayConnection,
                static_cast<Aspect_Drawable>(canvas->GetXWindow()),
                reinterpret_cast<Aspect_FBConfig>(canvas->GetEGLConfig()));
#else
            auto nativeWindow = new Xw_Window(
                displayConnection,
                static_cast<Aspect_Drawable>(canvas->GetXWindow()),
                static_cast<wxGLCanvasImpl*>(canvas)->GetGLXFBConfig() != nullptr
                    ? static_cast<wxGLCanvasImpl*>(canvas)->GetGLXFBConfig()[0]
                    : nullptr);
#endif
            view_->SetWindow(nativeWindow);
            view_->SetBackgroundColor(Quantity_NOC_DARKSLATEBLUE);

            context_ = new AIS_InteractiveContext(viewer_);
            view_->SetProj(1.0, -1.0, 1.0);

            // AIS_Manipulator::Attach() requires a non-null attached object
            // internally (see ObjectTransformation()), but this proxy is
            // never displayed or transformed: it only exists to satisfy
            // that requirement. The manipulator itself is used purely as a
            // visual + drag-math helper (ObjectTransformation()) - the
            // resulting delta is applied to the SceneService through
            // setTransform(); the scene stays the single source of truth for
            // every model's transform. setGizmoTarget() attaches
            // a fresh AIS_Manipulator to this same proxy every time the
            // active tool changes; see its comment for why.
            TopoDS_Vertex dummyVertex;
            BRep_Builder  builder;
            builder.MakeVertex(dummyVertex, gp_Pnt(0, 0, 0), Precision::Confusion());
            manipulatorProxy_ = new AIS_Shape(dummyVertex);
        }

        void load(const std::filesystem::path& path)
        {
            const std::size_t index        = scene_.load(path);
            auto              presentation = new AIS_Shape(scene_.models()[index].shape.value());
            presentations_.push_back(presentation);

            context_->Display(presentation, false);
            context_->SetDisplayMode(presentation, AIS_Shaded, false);
            context_->UpdateCurrentViewer();
            view_->FitAll(0.1, true);
        }

        void clear()
        {
            context_->RemoveAll(false);
            presentations_.clear();
            scene_.clear();
            view_->Redraw();
        }

        bool save(const std::filesystem::path& path) const
        {
            return scene_.saveSTL(path);
        }

        void resize()
        {
            view_->MustBeResized();
        }

        void paint()
        {
            view_->Redraw();
        }

        void rotate(const wxPoint& point)
        {
            view_->Rotation(point.x, point.y);
        }

        void startRotation(const wxPoint& point)
        {
            view_->StartRotation(point.x, point.y);
        }

        void pan(int dx, int dy)
        {
            view_->Pan(dx, dy, 1.0, false);
        }

        void zoom(int direction)
        {
            scale_ *= direction > 0 ? 1.15 : 0.85;
            scale_ = std::clamp(scale_, 0.05, 20.0);
            view_->SetScale(scale_);
        }

        // Applies an absolute transform to a target in the scene (see
        // SceneService::setTransform()) and mirrors it on the matching
        // presentations.
        void setTransform(int targetIndex, const geometry::occt::Transform& transform)
        {
            if (!scene_.setTransform(targetIndex, transform))
                return;

            const TopLoc_Location location(geometry::occt::toTrsf(transform));
            if (targetIndex == application::service::SceneService::kAllModels)
            {
                for (const auto& presentation : presentations_)
                    context_->SetLocation(presentation, location);
            }
            else
            {
                context_->SetLocation(presentations_[static_cast<std::size_t>(targetIndex)], location);
            }

            repositionGizmoIfVisible();
            view_->Redraw();
        }

        // Shows/hides/repositions the gizmo for `targetIndex` and enables
        // only the parts relevant to `tool`. targetIndex < 0 or out of
        // range always hides it (no single meaningful pivot for "all").
        void setGizmoTarget(int targetIndex, ModelViewerPanel::GizmoTool tool)
        {
            gizmoTargetIndex_ = targetIndex;
            gizmoTool_        = tool;

            if (context_->IsDisplayed(manipulator_))
            {
                context_->Deactivate(manipulator_);
                context_->Erase(manipulator_, false);
            }

            const bool show = tool != ModelViewerPanel::GizmoTool::None && targetIndex >= 0 &&
                              static_cast<std::size_t>(targetIndex) < scene_.size();

            if (!show)
            {
                view_->Redraw();
                return;
            }

            // Rebuilt from scratch every time, rather than SetPart()'d in
            // place on the existing instance: AIS_Manipulator only ever
            // computes a part's selection data (e.g. the rotation rings)
            // when that part is enabled on a *freshly attached* manipulator
            // before its first Display(). Toggling SetPart() on one that
            // was already displayed with a different set of parts left its
            // cached selection data stale - activating a mode whose
            // geometry was never actually (re)computed segfaulted inside
            // AIS_Manipulator::ComputeSelection.
            manipulator_ = new AIS_Manipulator();
            manipulator_->Attach(manipulatorProxy_,
                                 AIS_Manipulator::OptionsForAttach()
                                     .SetAdjustPosition(false)
                                     .SetAdjustSize(false)
                                     .SetEnableModes(false));
            manipulator_->SetModeActivationOnDetection(true);
            manipulator_->SetZoomPersistence(true);
            manipulator_->SetSize(80.f);

            const bool moveActive = tool == ModelViewerPanel::GizmoTool::Move;
            manipulator_->SetPart(AIS_MM_Scaling, false);
            manipulator_->SetPart(AIS_MM_Translation, moveActive);
            manipulator_->SetPart(AIS_MM_TranslationPlane, moveActive);
            manipulator_->SetPart(AIS_MM_Rotation, !moveActive);

            repositionGizmoIfVisible();

            context_->Display(manipulator_, false);
            if (moveActive)
            {
                context_->Activate(manipulator_, AIS_MM_Translation);
                context_->Activate(manipulator_, AIS_MM_TranslationPlane);
            }
            else
            {
                context_->Activate(manipulator_, AIS_MM_Rotation);
            }

            view_->Redraw();
        }

        // Starts a gizmo drag if (x, y) currently hovers an active handle.
        // Returns false (and does nothing) otherwise, so the caller can
        // fall back to camera rotation / model picking.
        bool startGizmoDragIfHit(const wxPoint& point)
        {
            if (gizmoTool_ == ModelViewerPanel::GizmoTool::None || gizmoTargetIndex_ < 0)
                return false;

            context_->MoveTo(point.x, point.y, view_, false);
            if (!manipulator_->HasActiveMode())
                return false;

            dragStart_ = scene_.transform(gizmoTargetIndex_);

            manipulator_->StartTransform(point.x, point.y, view_);
            gizmoDragging_ = true;
            return true;
        }

        // Applies the drag delta accumulated since StartTransform() to the
        // target's transform at drag start, through setTransform() so the
        // scene stays the single source of truth. No-op if no drag is in
        // progress.
        void updateGizmoDrag(const wxPoint& point)
        {
            if (!gizmoDragging_)
                return;

            gp_Trsf delta;
            if (!manipulator_->ObjectTransformation(point.x, point.y, view_, delta))
                return;

            geometry::occt::Transform transform = dragStart_;

            const AIS_ManipulatorMode mode = manipulator_->ActiveMode();
            if (mode == AIS_MM_Translation || mode == AIS_MM_TranslationPlane)
                transform = geometry::occt::withTranslationDelta(dragStart_, delta.TranslationPart());
            else if (mode == AIS_MM_Rotation)
                transform = geometry::occt::withRotationDelta(dragStart_, manipulator_->ActiveAxisIndex(), delta);

            setTransform(gizmoTargetIndex_, transform);
        }

        // Ends the current gizmo drag, if any.
        void stopGizmoDrag()
        {
            if (!gizmoDragging_)
                return;
            manipulator_->StopTransform(true);
            gizmoDragging_ = false;
        }

        [[nodiscard]] bool isGizmoDragging() const noexcept
        {
            return gizmoDragging_;
        }

      private:
        // Moves the gizmo to the current target's pivot - which is always
        // exactly (moveX, moveY, moveZ), since a Transform rotates each
        // model around its own local origin before translating it there.
        // World-aligned axes only for now (gp::DZ()/gp::DX()); a future
        // "local" orientation mode would build this gp_Ax2 from the
        // target's own rotation instead.
        void repositionGizmoIfVisible()
        {
            if (gizmoTool_ == ModelViewerPanel::GizmoTool::None || gizmoTargetIndex_ < 0 ||
                static_cast<std::size_t>(gizmoTargetIndex_) >= scene_.size())
                return;

            const auto transform = scene_.transform(gizmoTargetIndex_);
            manipulator_->SetPosition(
                gp_Ax2(gp_Pnt(transform.moveX, transform.moveY, transform.moveZ), gp::DZ(), gp::DX()));
        }

        application::service::SceneService& scene_;

        occ::handle<V3d_Viewer>             viewer_;
        occ::handle<V3d_View>               view_;
        occ::handle<AIS_InteractiveContext> context_;
        // One presentation per scene model, at the same index.
        std::vector<occ::handle<AIS_Shape>> presentations_;
        double                              scale_ = 1.0;

        occ::handle<AIS_Manipulator> manipulator_;
        occ::handle<AIS_Shape>       manipulatorProxy_;
        int                          gizmoTargetIndex_ = -1;
        ModelViewerPanel::GizmoTool  gizmoTool_        = ModelViewerPanel::GizmoTool::None;
        bool                         gizmoDragging_    = false;
        geometry::occt::Transform    dragStart_;
    };

    ModelViewerPanel::ModelViewerPanel(wxWindow* parent)
        : wxPanel(parent, wxID_ANY)
    {
        wxGLAttributes attributes;
        attributes.PlatformDefaults().RGBA().DoubleBuffer().Depth(24).Stencil(8).EndList();

        canvas_  = new wxGLCanvas(this, attributes, wxID_ANY, wxDefaultPosition,
                                  wxDefaultSize, wxFULL_REPAINT_ON_RESIZE);
        context_ = std::make_unique<wxGLContext>(canvas_);

        auto* sizer = new wxBoxSizer(wxVERTICAL);
        sizer->Add(canvas_, 1, wxEXPAND);
        SetSizer(sizer);

        canvas_->Bind(wxEVT_PAINT, &ModelViewerPanel::onPaint, this);
        canvas_->Bind(wxEVT_SIZE, &ModelViewerPanel::onSize, this);
        canvas_->Bind(wxEVT_LEFT_DOWN, &ModelViewerPanel::onMouseDown, this);
        canvas_->Bind(wxEVT_LEFT_UP, &ModelViewerPanel::onMouseUp, this);
        canvas_->Bind(wxEVT_MIDDLE_DOWN, &ModelViewerPanel::onMouseDown, this);
        canvas_->Bind(wxEVT_MIDDLE_UP, &ModelViewerPanel::onMouseUp, this);
        canvas_->Bind(wxEVT_MOTION, &ModelViewerPanel::onMouseMove, this);
        canvas_->Bind(wxEVT_MOUSEWHEEL, &ModelViewerPanel::onMouseWheel, this);

        // wxBG_STYLE_PAINT must be set on the canvas itself, not just the
        // wrapping panel: it is canvas_ that OpenGL paints into. Without
        // this (and the no-op erase handler below), GTK still erases the
        // canvas to its default background colour between GL frames during
        // rapid interactive repaint (dragging to orbit/pan), which showed
        // up as the view flickering as if its brightness were changing.
        canvas_->SetBackgroundStyle(wxBG_STYLE_PAINT);
        canvas_->Bind(wxEVT_ERASE_BACKGROUND, [](wxEraseEvent&) { });
        SetBackgroundStyle(wxBG_STYLE_PAINT);
    }

    ModelViewerPanel::~ModelViewerPanel() = default;

    void ModelViewerPanel::newFile()
    {
        try
        {
            initializeViewer();
            viewer_->clear();
            notifyModelStateChanged();
        }
        catch (const std::exception& error)
        {
            wxLogError("Unable to create a new model: %s", error.what());
        }
    }

    void ModelViewerPanel::openFile()
    {
        const auto path = ui::dialogs::openFile(
            this, "Import 3D model", "STL files (*.stl)|*.stl|All files (*.*)|*.*");
        if (!path)
            return;

        loadFile(toPath(*path));
    }

    void ModelViewerPanel::saveFile()
    {
        wxFileDialog dialog(
            this,
            "Export 3D model",
            "",
            "model.stl",
            "STL files (*.stl)|*.stl|All files (*.*)|*.*",
            wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
        if (dialog.ShowModal() != wxID_OK)
            return;

        exportFile(toPath(dialog.GetPath()));
    }

    bool ModelViewerPanel::loadFile(const std::filesystem::path& path)
    {
        try
        {
            initializeViewer();
            viewer_->load(path);
            viewer_->paint();
            notifyModelStateChanged();
            return true;
        }
        catch (const std::exception& error)
        {
            wxLogError("Unable to import the model: %s", error.what());
            return false;
        }
    }

    bool ModelViewerPanel::exportFile(const std::filesystem::path& path)
    {
        try
        {
            initializeViewer();
            if (viewer_->save(path))
                return true;
            wxLogError("Unable to export the model.");
        }
        catch (const std::exception& error)
        {
            wxLogError("Unable to export the model: %s", error.what());
        }
        return false;
    }

    void ModelViewerPanel::initializeViewer()
    {
        if (initialized_)
            return;

        // The only place that binds wx's own GL context. From here on,
        // OpenGl_GraphicDriver (created inside Viewer) manages and binds
        // its own separate GL context internally on every View::Redraw().
        // Re-binding wx's context before each redraw/swap - as this used
        // to do everywhere below - made rendering alternate between two
        // independently configured contexts (only one of which ever had
        // OCCT's lighting set up), which showed up as the view randomly
        // going dark and flickering during interaction.
        canvas_->SetCurrent(*context_);
        viewer_      = std::make_unique<Viewer>(canvas_, scene_);
        initialized_ = true;
    }

    bool ModelViewerPanel::hasModel() const
    {
        return !scene_.empty();
    }

    std::vector<wxString> ModelViewerPanel::GetModelNames() const
    {
        std::vector<wxString> names;
        names.reserve(scene_.size());
        for (const auto& name : scene_.names())
            names.push_back(wxString::FromUTF8(name));
        return names;
    }

    void ModelViewerPanel::setTransform(int    targetIndex,
                                        double moveXmm,
                                        double moveYmm,
                                        double moveZmm,
                                        double rotateXdeg,
                                        double rotateYdeg,
                                        double rotateZdeg)
    {
        if (!initialized_ || viewer_ == nullptr)
            return;

        viewer_->setTransform(targetIndex, {moveXmm, moveYmm, moveZmm, rotateXdeg, rotateYdeg, rotateZdeg});
    }

    ModelViewerPanel::TransformValues ModelViewerPanel::GetTransform(int targetIndex) const
    {
        return scene_.transform(targetIndex);
    }

    void ModelViewerPanel::SetGizmoState(int targetIndex, GizmoTool tool)
    {
        if (!initialized_ || viewer_ == nullptr)
            return;

        viewer_->setGizmoTarget(targetIndex, tool);
    }

    void ModelViewerPanel::SetOnGizmoChanged(std::function<void()> callback)
    {
        on_gizmo_changed_ = std::move(callback);
    }

    void ModelViewerPanel::SetOnModelStateChanged(std::function<void(bool)> callback)
    {
        on_model_state_changed_ = std::move(callback);
    }

    void ModelViewerPanel::notifyModelStateChanged()
    {
        if (on_model_state_changed_)
            on_model_state_changed_(hasModel());
    }

    void ModelViewerPanel::onPaint(wxPaintEvent& event)
    {
        wxPaintDC paintDc(canvas_);
        (void) event;

        try
        {
            initializeViewer();
            viewer_->paint();
        }
        catch (const std::exception& error)
        {
            wxLogError("Unable to initialize the model viewer: %s", error.what());
        }
    }

    void ModelViewerPanel::onSize(wxSizeEvent& event)
    {
        if (initialized_)
            viewer_->resize();
        event.Skip();
    }

    void ModelViewerPanel::onMouseDown(wxMouseEvent& event)
    {
        last_mouse_position_ = event.GetPosition();
        if (event.LeftDown() && initialized_)
        {
            if (viewer_->startGizmoDragIfHit(last_mouse_position_))
            {
                gizmo_dragging_ = true;
                canvas_->CaptureMouse();
                return;
            }

            rotating_ = true;
            viewer_->startRotation(last_mouse_position_);
        }
        if (event.MiddleDown())
            panning_ = true;
        canvas_->CaptureMouse();
    }

    void ModelViewerPanel::onMouseUp(wxMouseEvent& event)
    {
        if (event.LeftUp())
        {
            if (gizmo_dragging_ && initialized_)
                viewer_->stopGizmoDrag();
            gizmo_dragging_ = false;
            rotating_       = false;
        }
        if (event.MiddleUp())
            panning_ = false;
        if (!rotating_ && !panning_ && canvas_->HasCapture())
            canvas_->ReleaseMouse();
    }

    void ModelViewerPanel::onMouseMove(wxMouseEvent& event)
    {
        if (!initialized_)
            return;

        const wxPoint position = event.GetPosition();

        // Note: this deliberately does no gizmo hit-testing while just
        // hovering (no button down). Continuously calling into OCCT's
        // AIS_InteractiveContext::MoveTo() on every pixel of mouse motion
        // appeared to trigger an internal redraw of its own. Detection is
        // instead redone fresh at the moment of each click (see
        // startGizmoDragIfHit()), which is all that is needed for dragging
        // to work; the only thing lost is highlighting a handle before it
        // is clicked.
        if (gizmo_dragging_)
        {
            // updateGizmoDrag() already redraws (via setTransform()) when
            // it successfully applies a delta; no need to redraw again.
            viewer_->updateGizmoDrag(position);
            last_mouse_position_ = position;
            if (on_gizmo_changed_)
                on_gizmo_changed_();
            return;
        }

        if (!rotating_ && !panning_)
            return;

        const int dx = position.x - last_mouse_position_.x;
        const int dy = position.y - last_mouse_position_.y;

        if (rotating_)
            viewer_->rotate(position);
        else if (panning_)
            viewer_->pan(dx, dy);

        viewer_->paint();
        last_mouse_position_ = position;
    }

    void ModelViewerPanel::onMouseWheel(wxMouseEvent& event)
    {
        if (!initialized_)
            return;
        viewer_->zoom(event.GetWheelRotation() / 100);
        viewer_->paint();
    }
} // namespace ui::framework::occt
