#include "model_viewer_panel.hpp"

#include "geometry/occt/shape.hpp"
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
#include <StlAPI_Writer.hxx>
#include <TopoDS_Vertex.hxx>
#if defined(_WIN32)
#include <WNT_Window.hxx>
#else
#include <Xw_Window.hxx>
#endif
#include <TopLoc_Location.hxx>
#include <V3d_View.hxx>
#include <V3d_Viewer.hxx>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <gp_Ax1.hxx>
#include <gp_Ax2.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>
#include <stdexcept>
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
    class ModelViewerPanel::Viewer
    {
      public:
        explicit Viewer(wxGLCanvas* canvas)
        {
            auto displayConnection = new Aspect_DisplayConnection();
            auto graphicDriver     = new OpenGl_GraphicDriver(displayConnection);
            viewer_                = new V3d_Viewer(graphicDriver);
            viewer_->SetDefaultLights();
            viewer_->SetLightOn();

            view_ = viewer_->CreateView();
#if defined(_WIN32)
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
            // resulting delta is applied to our own ModelEntry fields
            // through setTransform(), which stays the single source of
            // truth for every model's transform. setGizmoTarget() attaches
            // a fresh AIS_Manipulator to this same proxy every time the
            // active tool changes; see its comment for why.
            TopoDS_Vertex dummyVertex;
            BRep_Builder  builder;
            builder.MakeVertex(dummyVertex, gp_Pnt(0, 0, 0), Precision::Confusion());
            manipulatorProxy_ = new AIS_Shape(dummyVertex);
        }

        void load(const std::filesystem::path& path)
        {
            const auto shape        = geometry::occt::Shape::fromSTL(path);
            auto       presentation = new AIS_Shape(shape.value());

            ModelEntry entry;
            entry.name         = uniqueName(path.filename().string());
            entry.presentation = presentation;
            entry.shape        = shape.value();
            models_.push_back(entry);

            context_->Display(presentation, false);
            context_->SetDisplayMode(presentation, AIS_Shaded, false);
            context_->UpdateCurrentViewer();
            view_->FitAll(0.1, true);
        }

        void clear()
        {
            context_->RemoveAll(false);
            models_.clear();
            view_->Redraw();
        }

        bool save(const std::filesystem::path& path) const
        {
            if (models_.empty())
                return false;

            const std::string filename = path.string();
            StlAPI_Writer     writer;
            // TODO: Save all the shapes in the vector instead of just the first one.
            return writer.Write(models_.front().shape, filename.c_str());
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

        [[nodiscard]] bool hasShape() const noexcept
        {
            return !models_.empty();
        }

        // Display names of every currently loaded model, in load order.
        [[nodiscard]] std::vector<wxString> modelNames() const
        {
            std::vector<wxString> names;
            names.reserve(models_.size());
            for (const auto& entry : models_)
                names.push_back(entry.name);
            return names;
        }

        // Returns the move/rotate values currently applied to a target.
        // All-zero if targetIndex is -1 or out of range.
        [[nodiscard]] ModelViewerPanel::TransformValues transformFor(int targetIndex) const
        {
            if (targetIndex < 0 || static_cast<std::size_t>(targetIndex) >= models_.size())
                return {};

            const auto& entry = models_[static_cast<std::size_t>(targetIndex)];
            return {entry.moveX, entry.moveY, entry.moveZ, entry.rotateX, entry.rotateY, entry.rotateZ};
        }

        // Applies an absolute move (millimeters) and rotation (degrees,
        // around the origin) to a target, replacing whatever transform was
        // previously applied to it. targetIndex selects which one: -1
        // applies the same absolute transform to every loaded model,
        // otherwise it is an index into models_.
        void setTransform(int    targetIndex,
                          double moveX,
                          double moveY,
                          double moveZ,
                          double rotXDeg,
                          double rotYDeg,
                          double rotZDeg)
        {
            if (models_.empty())
                return;

            constexpr double kDegToRad = M_PI / 180.0;

            gp_Trsf rotationX;
            rotationX.SetRotation(gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(1, 0, 0)), rotXDeg * kDegToRad);
            gp_Trsf rotationY;
            rotationY.SetRotation(gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(0, 1, 0)), rotYDeg * kDegToRad);
            gp_Trsf rotationZ;
            rotationZ.SetRotation(gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1)), rotZDeg * kDegToRad);

            gp_Trsf translation;
            translation.SetTranslation(gp_Vec(moveX, moveY, moveZ));

            const TopLoc_Location location(translation * rotationZ * rotationY * rotationX);

            const auto apply = [&](ModelEntry& entry) {
                entry.moveX   = moveX;
                entry.moveY   = moveY;
                entry.moveZ   = moveZ;
                entry.rotateX = rotXDeg;
                entry.rotateY = rotYDeg;
                entry.rotateZ = rotZDeg;
                context_->SetLocation(entry.presentation, location);
            };

            if (targetIndex < 0)
            {
                for (auto& entry : models_)
                    apply(entry);
            }
            else if (static_cast<std::size_t>(targetIndex) < models_.size())
            {
                apply(models_[static_cast<std::size_t>(targetIndex)]);
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
                              static_cast<std::size_t>(targetIndex) < models_.size();

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

            const auto start   = transformFor(gizmoTargetIndex_);
            dragStartMoveX_    = start.moveX;
            dragStartMoveY_    = start.moveY;
            dragStartMoveZ_    = start.moveZ;
            dragStartRotateX_  = start.rotateX;
            dragStartRotateY_  = start.rotateY;
            dragStartRotateZ_  = start.rotateZ;

            manipulator_->StartTransform(point.x, point.y, view_);
            gizmoDragging_ = true;
            return true;
        }

        // Applies the drag delta accumulated since StartTransform() to the
        // target's stored move/rotate values via setTransform(), keeping
        // it the single source of truth. No-op if no drag is in progress.
        void updateGizmoDrag(const wxPoint& point)
        {
            if (!gizmoDragging_)
                return;

            gp_Trsf delta;
            if (!manipulator_->ObjectTransformation(point.x, point.y, view_, delta))
                return;

            double moveX = dragStartMoveX_, moveY = dragStartMoveY_, moveZ = dragStartMoveZ_;
            double rotateX = dragStartRotateX_, rotateY = dragStartRotateY_,
                   rotateZ = dragStartRotateZ_;

            const AIS_ManipulatorMode mode = manipulator_->ActiveMode();
            if (mode == AIS_MM_Translation || mode == AIS_MM_TranslationPlane)
            {
                const gp_XYZ translation = delta.TranslationPart();
                moveX += translation.X();
                moveY += translation.Y();
                moveZ += translation.Z();
            }
            else if (mode == AIS_MM_Rotation)
            {
                gp_XYZ axis;
                double angleRad = 0.0;
                if (delta.GetRotation(axis, angleRad))
                {
                    const int     axisIndex = manipulator_->ActiveAxisIndex();
                    const gp_Dir  expected  = axisIndex == 0   ? gp::DX()
                                             : axisIndex == 1 ? gp::DY()
                                                                : gp::DZ();
                    const double  sign      = axis.Dot(expected.XYZ()) >= 0.0 ? 1.0 : -1.0;
                    const double  deltaDeg  = angleRad * (180.0 / M_PI) * sign;

                    if (axisIndex == 0)
                        rotateX += deltaDeg;
                    else if (axisIndex == 1)
                        rotateY += deltaDeg;
                    else
                        rotateZ += deltaDeg;
                }
            }

            setTransform(gizmoTargetIndex_, moveX, moveY, moveZ, rotateX, rotateY, rotateZ);
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
        // exactly (moveX, moveY, moveZ), since setTransform() rotates each
        // model around its own local origin before translating it there.
        // World-aligned axes only for now (gp::DZ()/gp::DX()); a future
        // "local" orientation mode would build this gp_Ax2 from the
        // target's own rotation instead.
        void repositionGizmoIfVisible()
        {
            if (gizmoTool_ == ModelViewerPanel::GizmoTool::None || gizmoTargetIndex_ < 0 ||
                static_cast<std::size_t>(gizmoTargetIndex_) >= models_.size())
                return;

            const auto& entry = models_[static_cast<std::size_t>(gizmoTargetIndex_)];
            manipulator_->SetPosition(gp_Ax2(gp_Pnt(entry.moveX, entry.moveY, entry.moveZ),
                                             gp::DZ(), gp::DX()));
        }

        struct ModelEntry
        {
            wxString               name;
            occ::handle<AIS_Shape> presentation;
            TopoDS_Shape           shape;
            double                 moveX   = 0.0;
            double                 moveY   = 0.0;
            double                 moveZ   = 0.0;
            double                 rotateX = 0.0;
            double                 rotateY = 0.0;
            double                 rotateZ = 0.0;
        };

        // Disambiguates two models loaded from files with the same name
        // (e.g. imported from different folders) so the target dropdown
        // never shows duplicate entries.
        [[nodiscard]] wxString uniqueName(const wxString& base) const
        {
            wxString candidate = base;
            for (int suffix = 2; std::any_of(models_.begin(), models_.end(),
                                             [&](const ModelEntry& entry) { return entry.name == candidate; });
                ++suffix)
                candidate = wxString::Format("%s (%d)", base, suffix);
            return candidate;
        }

        occ::handle<V3d_Viewer>             viewer_;
        occ::handle<V3d_View>               view_;
        occ::handle<AIS_InteractiveContext> context_;
        std::vector<ModelEntry>             models_;
        double                              scale_ = 1.0;

        occ::handle<AIS_Manipulator> manipulator_;
        occ::handle<AIS_Shape>       manipulatorProxy_;
        int                          gizmoTargetIndex_ = -1;
        ModelViewerPanel::GizmoTool  gizmoTool_        = ModelViewerPanel::GizmoTool::None;
        bool                         gizmoDragging_    = false;
        double                       dragStartMoveX_   = 0.0;
        double                       dragStartMoveY_   = 0.0;
        double                       dragStartMoveZ_   = 0.0;
        double                       dragStartRotateX_ = 0.0;
        double                       dragStartRotateY_ = 0.0;
        double                       dragStartRotateZ_ = 0.0;
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
        SetBackgroundStyle(wxBG_STYLE_PAINT);
    }

    ModelViewerPanel::~ModelViewerPanel() = default;

    void ModelViewerPanel::newFile()
    {
        try
        {
            initializeViewer();
            canvas_->SetCurrent(*context_);
            viewer_->clear();
            canvas_->SwapBuffers();
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

        try
        {
            initializeViewer();
            canvas_->SetCurrent(*context_);
            viewer_->load(std::filesystem::path(path->ToStdString()));
            viewer_->paint();
            canvas_->SwapBuffers();
            notifyModelStateChanged();
        }
        catch (const std::exception& error)
        {
            wxLogError("Unable to import the model: %s", error.what());
        }
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

        try
        {
            initializeViewer();
            if (!viewer_->save(std::filesystem::path(dialog.GetPath().ToStdString())))
                wxLogError("Unable to export the model.");
        }
        catch (const std::exception& error)
        {
            wxLogError("Unable to export the model: %s", error.what());
        }
    }

    void ModelViewerPanel::initializeViewer()
    {
        if (initialized_)
            return;

        canvas_->SetCurrent(*context_);
        viewer_      = std::make_unique<Viewer>(canvas_);
        initialized_ = true;
    }

    bool ModelViewerPanel::hasModel() const
    {
        return initialized_ && viewer_ != nullptr && viewer_->hasShape();
    }

    std::vector<wxString> ModelViewerPanel::GetModelNames() const
    {
        if (!initialized_ || viewer_ == nullptr)
            return {};
        return viewer_->modelNames();
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

        canvas_->SetCurrent(*context_);
        viewer_->setTransform(
            targetIndex, moveXmm, moveYmm, moveZmm, rotateXdeg, rotateYdeg, rotateZdeg);
        canvas_->SwapBuffers();
    }

    ModelViewerPanel::TransformValues ModelViewerPanel::GetTransform(int targetIndex) const
    {
        if (!initialized_ || viewer_ == nullptr)
            return {};
        return viewer_->transformFor(targetIndex);
    }

    void ModelViewerPanel::SetGizmoState(int targetIndex, GizmoTool tool)
    {
        if (!initialized_ || viewer_ == nullptr)
            return;

        canvas_->SetCurrent(*context_);
        viewer_->setGizmoTarget(targetIndex, tool);
        canvas_->SwapBuffers();
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
            canvas_->SetCurrent(*context_);
            viewer_->paint();
            canvas_->SwapBuffers();
        }
        catch (const std::exception& error)
        {
            wxLogError("Unable to initialize the model viewer: %s", error.what());
        }
    }

    void ModelViewerPanel::onSize(wxSizeEvent& event)
    {
        if (initialized_)
        {
            canvas_->SetCurrent(*context_);
            viewer_->resize();
        }
        event.Skip();
    }

    void ModelViewerPanel::onMouseDown(wxMouseEvent& event)
    {
        last_mouse_position_ = event.GetPosition();
        if (event.LeftDown() && initialized_)
        {
            canvas_->SetCurrent(*context_);

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
            {
                canvas_->SetCurrent(*context_);
                viewer_->stopGizmoDrag();
            }
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
        // caused a visible flicker (it appears to trigger an internal
        // redraw of its own, independent of - and out of sync with - our
        // own paint()/SwapBuffers() calls). Detection is instead redone
        // fresh at the moment of each click (see startGizmoDragIfHit()),
        // which is all that is needed for dragging to work; the only
        // thing lost is highlighting a handle before it is clicked.
        if (gizmo_dragging_)
        {
            canvas_->SetCurrent(*context_);
            viewer_->updateGizmoDrag(position);
            viewer_->paint();
            canvas_->SwapBuffers();
            last_mouse_position_ = position;
            if (on_gizmo_changed_)
                on_gizmo_changed_();
            return;
        }

        if (!rotating_ && !panning_)
            return;

        const int dx = position.x - last_mouse_position_.x;
        const int dy = position.y - last_mouse_position_.y;
        canvas_->SetCurrent(*context_);

        if (rotating_)
            viewer_->rotate(position);
        else if (panning_)
            viewer_->pan(dx, dy);

        viewer_->paint();
        canvas_->SwapBuffers();
        last_mouse_position_ = position;
    }

    void ModelViewerPanel::onMouseWheel(wxMouseEvent& event)
    {
        if (!initialized_)
            return;
        canvas_->SetCurrent(*context_);
        viewer_->zoom(event.GetWheelRotation() / 100);
        viewer_->paint();
        canvas_->SwapBuffers();
    }
} // namespace ui::framework::occt
