#include "model_viewer_panel.hpp"

#include "application/tasks/model_tasks.hpp"
#include "geometry/occt/mesh_pick_index.hpp"
#include "geometry/occt/transform.hpp"
#include "platform/benchmark/benchmark.hpp"
#include "platform/logger/logger.hpp"
#include "platform/threading/dispatcher.hpp"
#include "ui/dialogs/dialogs.hpp"
#include "ui/framework/wx/locale/locale.hpp"

#include <AIS_DisplayMode.hxx>
#include <AIS_InteractiveContext.hxx>
#include <AIS_Manipulator.hxx>
#include <AIS_ManipulatorMode.hxx>
#include <AIS_Shape.hxx>
#include <Aspect_DisplayConnection.hxx>
#include <Aspect_GraphicDeviceDefinitionError.hxx>
#include <BRep_Builder.hxx>
#include <Bnd_Box.hxx>
#include <Graphic3d_Camera.hxx>
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
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <gp.hxx>
#include <gp_Ax2.hxx>
#include <gp_Dir.hxx>
#include <gp_Lin.hxx>
#include <gp_Pnt.hxx>
#include <gp_Trsf.hxx>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <utility>
#include <vector>
#include <wx/dcclient.h>
#include <wx/log.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/stdpaths.h>
#if defined(__WXGTK__)
#include <wx/gtk/glcanvas.h>
#endif

using ui::framework::wx::locale::tr;

namespace ui::framework::occt
{
    namespace
    {
        // AIS_InteractiveContext::Display()'s "no selection mode": models are
        // picked by Viewer::pick(), not OCCT's selection.
        constexpr int kNoSelection = -1;

        // A press and release closer than this (pixels, on each axis) is a
        // click; further, the press was an orbit.
        constexpr int kClickSlop = 4;

        // How far a selected model's colour goes toward white.
        constexpr double kHighlightLightening = 0.35;

        // wxString -> path without going through the narrow locale encoding
        // (ToStdString()), which loses non-ASCII characters on Windows.
        std::filesystem::path toPath(const wxString& value)
        {
            const std::string utf8 = value.utf8_string();
            return std::filesystem::path(std::u8string(utf8.begin(), utf8.end()));
        }

        using MeshPrecision = geometry::occt::Shape::Precision;

        MeshPrecision toPrecision(const std::string& value)
        {
            return value == "double" ? MeshPrecision::Double : MeshPrecision::Single;
        }

        void declareSettings()
        {
            using platform::settings::Kind;
            auto& settings = platform::settings::SettingsRegistry::getInstance();
            settings.add({kMeshPrecisionSetting, "settings.section.import", "settings.import.precision", "settings.import.precision_note", Kind::Choice, "single", {{"single", "settings.import.precision_single"}, {"double", "settings.import.precision_double"}}});
            settings.add({kComputeBRepSetting, "settings.section.import", "settings.import.compute_brep", "settings.import.compute_brep_note", Kind::Toggle, "0", {}});
        }
    } // namespace

    // The OCCT viewer, and the SceneEditor through which tasks commit their
    // changes: every change goes to the scene and to its presentation together.
    class ModelViewerPanel::Viewer final : public application::tasks::SceneEditor
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
            try
            {
                view_->SetWindow(nativeWindow);
            }
            catch (const Aspect_GraphicDeviceDefinitionError&)
            {
                graphicDriver->ChangeOptions().contextNoAccel = true;
                view_->SetWindow(nativeWindow);
            }
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

        void setOnModelsChanged(std::function<void()> callback)
        {
            onModelsChanged_ = std::move(callback);
        }

        // Whether the user is moving the camera or a gizmo right now.
        void setIsInteracting(std::function<bool()> callback)
        {
            isInteracting_ = std::move(callback);
        }

        void addModel(application::service::ModelId id,
                      const std::string&            baseName,
                      geometry::occt::Shape         shape,
                      platform::benchmark::Duration loadDuration) override
        {
            // Import time was logged by the import task; this adds the time
            // spent building the shaded presentation and fitting the view.
            const std::size_t index = scene_.add(baseName, std::move(shape), id, loadDuration);

            std::optional<platform::benchmark::Report> report;
            platform::benchmark::measure(&report, platform::benchmark::ALL, [&] {
                auto presentation = new AIS_Shape(scene_.models()[index].shape.value());
                // A mesh face has no surface to mesh again: draw its triangulation as is.
                presentation->Attributes()->SetAutoTriangulation(false);
                presentations_.push_back({presentation});

                // Straight to shaded, with no selection mode: displaying in the
                // default (wireframe) mode first builds a hidden presentation of
                // every edge, and OCCT's picking structures for a large mesh take
                // seconds and gigabytes on this thread. Clicks use a
                // MeshPickIndex built in the background instead (see pick()).
                context_->Display(presentation, AIS_Shaded, kNoSelection, false);
                context_->UpdateCurrentViewer();
                // A background import can land mid-orbit or mid-drag: don't
                // yank the camera away from the user then.
                if (!isInteracting_ || !isInteracting_())
                    view_->FitAll(0.1, true);
            });

            const auto& model   = scene_.models()[index];
            const auto  display = report->time.value_or(platform::benchmark::Duration::zero());
            platform::logger::clog("[viewer] displayed '", model.name, "' (", *report, "; import ",
                                   platform::benchmark::formatMilliseconds(model.loadDuration), ", total ",
                                   platform::benchmark::formatMilliseconds(model.loadDuration + display), ")");
            buildPickIndex(model.id);
            modelsChanged();
        }

        void clear() override
        {
            if (gizmoTargetIndex_ >= 0)
                setGizmoTarget(-1, ModelViewerPanel::GizmoTool::None);
            context_->RemoveAll(false);
            presentations_.clear();
            scene_.clear();
            pickIndices_.clear();
            view_->Redraw();
            modelsChanged();
        }

        void removeModel(application::service::ModelId id) override
        {
            if (const auto index = scene_.indexOf(id))
                removeAt(static_cast<int>(*index));
            pickIndices_.erase(id);
            modelsChanged();
        }

        void setBRep(application::service::ModelId id, geometry::occt::Shape brep) override
        {
            scene_.setBRep(id, std::move(brep)); // the mesh stays what is shown
        }

        void setShape(application::service::ModelId id, geometry::occt::Shape shape) override
        {
            const auto index = scene_.indexOf(id);
            if (!index || !scene_.setShape(id, std::move(shape)))
                return;
            // Same geometry: the placement (the presentation's location) stays.
            const auto& presentation = presentations_[*index].shape;
            presentation->SetShape(scene_.models()[*index].shape.value());
            context_->Redisplay(presentation, false);
            view_->Redraw();
            buildPickIndex(id); // new triangles
        }

        void setTransform(application::service::ModelId id, const geometry::occt::Transform& transform) override
        {
            if (const auto index = scene_.indexOf(id))
                setTransformAt(static_cast<int>(*index), transform);
        }

        void resize()
        {
            view_->MustBeResized();
        }

        // How background work hands its result to the UI thread (see
        // buildPickIndex()); it drops it once the panel is gone.
        void setRunOnUi(std::function<void(std::function<void()>)> runOnUi)
        {
            runOnUi_ = std::move(runOnUi);
        }

        // The visible model under a canvas point (the nearest one), or -1.
        // Ignores the gizmo. A model whose pick index is still being built
        // is hit by its bounding box meanwhile.
        [[nodiscard]] int pick(const wxPoint& point) const
        {
            double x = 0.0, y = 0.0, z = 0.0, dx = 0.0, dy = 0.0, dz = 0.0;
            view_->ConvertWithProj(point.x, point.y, x, y, z, dx, dy, dz);
            // Pointing away from the eye: the smallest parameter is the nearest hit.
            const gp_Lin ray(gp_Pnt(x, y, z), gp_Dir(dx, dy, dz));

            int    found = -1;
            double best  = std::numeric_limits<double>::infinity();
            for (std::size_t i = 0; i < presentations_.size(); ++i)
            {
                if (!presentations_[i].visible)
                    continue;
                const auto&           model = scene_.models()[i];
                std::optional<double> hit;
                if (const auto index = pickIndices_.find(model.id); index != pickIndices_.end() && index->second)
                {
                    // Into the model's own coordinates (a rigid move keeps distances).
                    hit = index->second->intersect(ray.Transformed(geometry::occt::toTrsf(model.transform).Inverted()));
                }
                else
                {
                    Bnd_Box box;
                    presentations_[i].shape->BoundingBox(box); // world space
                    if (!box.IsVoid() && !box.IsOut(ray))
                    {
                        double xMin = 0.0, yMin = 0.0, zMin = 0.0, xMax = 0.0, yMax = 0.0, zMax = 0.0;
                        box.Get(xMin, yMin, zMin, xMax, yMax, zMax);
                        const gp_XYZ centre((xMin + xMax) / 2.0, (yMin + yMax) / 2.0, (zMin + zMax) / 2.0);
                        hit = (centre - ray.Location().XYZ()).Dot(ray.Direction().XYZ());
                    }
                }
                if (hit && *hit < best)
                {
                    best  = *hit;
                    found = static_cast<int>(i);
                }
            }
            return found;
        }

        // Whether every model's pick index is built.
        [[nodiscard]] bool pickIndicesReady() const
        {
            return std::all_of(scene_.models().begin(), scene_.models().end(),
                               [this](const auto& model) { return pickIndices_.contains(model.id); });
        }

        // Draws a model in a lighter shade of its own colour, or back in it.
        void setHighlighted(int modelIndex, bool highlighted)
        {
            if (!isValidIndex(modelIndex))
                return;
            const auto& shape = presentations_[static_cast<std::size_t>(modelIndex)].shape;
            if (highlighted == shape->HasColor())
                return;
            if (!highlighted)
            {
                context_->UnsetColor(shape, false);
                return;
            }
            Quantity_Color base;
            shape->Color(base);
            const auto lighter = [](double c) { return c + (1.0 - c) * kHighlightLightening; };
            context_->SetColor(shape, Quantity_Color(lighter(base.Red()), lighter(base.Green()), lighter(base.Blue()), Quantity_TOC_RGB),
                               false);
        }

        [[nodiscard]] bool hasDetected() const
        {
            return context_->HasDetected();
        }

        [[nodiscard]] bool isHighlighted(int modelIndex) const
        {
            return isValidIndex(modelIndex) && presentations_[static_cast<std::size_t>(modelIndex)].shape->HasColor();
        }

        [[nodiscard]] wxPoint project(const gp_Pnt& point) const
        {
            int x = 0;
            int y = 0;
            view_->Convert(point.X(), point.Y(), point.Z(), x, y);
            return {x, y};
        }

        void fitAll()
        {
            view_->FitAll(0.1, true);
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

        void zoom(double notches)
        {
            scale_ *= std::pow(1.15, notches);
            scale_ = std::clamp(scale_, 0.05, 20.0);
            view_->SetScale(scale_);
        }

        // Visibility of every model, in load order (scene indices).
        [[nodiscard]] std::vector<bool> modelVisibility() const
        {
            std::vector<bool> visibility;
            visibility.reserve(presentations_.size());
            for (const auto& entry : presentations_)
                visibility.push_back(entry.visible);
            return visibility;
        }

        // Shows or hides one model in the viewport. Hidden models stay in the
        // scene (and keep their transform); out-of-range indices are ignored.
        void setModelVisibility(int modelIndex, bool visible)
        {
            if (!isValidIndex(modelIndex))
                return;

            auto& entry   = presentations_[static_cast<std::size_t>(modelIndex)];
            entry.visible = visible;
            if (visible)
                context_->Display(entry.shape, AIS_Shaded, kNoSelection, false);
            else
                context_->Erase(entry.shape, false);
            view_->Redraw();
        }

        // Removes one model from the scene and the viewport; the following
        // models shift down by one index. Hides the gizmo if it was on it.
        void removeAt(int modelIndex)
        {
            if (!isValidIndex(modelIndex))
                return;

            if (gizmoTargetIndex_ == modelIndex)
                setGizmoTarget(-1, ModelViewerPanel::GizmoTool::None);

            const auto index = static_cast<std::size_t>(modelIndex);
            context_->Remove(presentations_[index].shape, false);
            presentations_.erase(presentations_.begin() + modelIndex);
            scene_.remove(modelIndex);
            if (gizmoTargetIndex_ > modelIndex)
                --gizmoTargetIndex_;
            view_->Redraw();
        }

        // Frames the camera on one model, without changing which models are
        // shown: FitAll() only considers displayed objects, so every other
        // visible model is erased for the fit, then the visibility restored.
        void focusModel(int modelIndex)
        {
            if (!isValidIndex(modelIndex))
                return;

            const auto target = static_cast<std::size_t>(modelIndex);
            for (const auto& entry : presentations_)
                if (entry.visible)
                    context_->Erase(entry.shape, false);

            context_->Display(presentations_[target].shape, AIS_Shaded, kNoSelection, false);
            view_->FitAll(0.1, true);

            for (const auto& entry : presentations_)
            {
                if (entry.visible)
                    context_->Display(entry.shape, AIS_Shaded, kNoSelection, false);
                else
                    context_->Erase(entry.shape, false);
            }
            view_->Redraw();
        }

        // Applies an absolute transform to a target in the scene (see
        // SceneService::setTransform()) and mirrors it on the matching
        // presentations.
        void setTransformAt(int targetIndex, const geometry::occt::Transform& transform)
        {
            if (!scene_.setTransform(targetIndex, transform))
                return;

            const TopLoc_Location location(geometry::occt::toTrsf(transform));
            if (targetIndex == application::service::SceneService::kAllModels)
            {
                for (const auto& entry : presentations_)
                    context_->SetLocation(entry.shape, location);
            }
            else
            {
                context_->SetLocation(presentations_[static_cast<std::size_t>(targetIndex)].shape, location);
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

            const bool show = tool != ModelViewerPanel::GizmoTool::None &&
                              (isValidIndex(targetIndex) || targetIndex == ModelViewerPanel::kSelectionTarget);

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
            if (!hasGizmoTarget())
                return false;

            context_->MoveTo(point.x, point.y, view_, false);
            if (!manipulator_->HasActiveMode())
            {
                // A model under the press would stay hover-highlighted (cyan).
                context_->ClearDetected(false);
                return false;
            }

            // A group's drag changes its offset; a model's, its placement.
            dragStart_ = gizmoTargetIndex_ == ModelViewerPanel::kSelectionTarget ? groupOffset_
                                                                                 : scene_.transform(gizmoTargetIndex_);

            manipulator_->StartTransform(point.x, point.y, view_);
            gizmoDragging_ = true;
            return true;
        }

        // The target's transform at drag start plus the drag delta
        // accumulated since StartTransform(); empty if no drag is in progress
        // or the manipulator has no delta. Changes nothing: the caller commits
        // it (as a move task) or previews it.
        [[nodiscard]] std::optional<geometry::occt::Transform> gizmoDragTransform(const wxPoint& point)
        {
            if (!gizmoDragging_)
                return std::nullopt;

            gp_Trsf delta;
            if (!manipulator_->ObjectTransformation(point.x, point.y, view_, delta))
                return std::nullopt;

            geometry::occt::Transform transform = dragStart_;

            const AIS_ManipulatorMode mode = manipulator_->ActiveMode();
            if (mode == AIS_MM_Translation || mode == AIS_MM_TranslationPlane)
                transform = geometry::occt::withTranslationDelta(dragStart_, delta.TranslationPart());
            else if (mode == AIS_MM_Rotation)
                transform = geometry::occt::withRotationDelta(dragStart_, manipulator_->ActiveAxisIndex(), delta);
            return transform;
        }

        [[nodiscard]] ModelViewerPanel::CameraState camera() const
        {
            const auto&  camera = view_->Camera();
            const gp_Pnt eye    = camera->Eye();
            const gp_Pnt center = camera->Center();
            const gp_Dir up     = camera->Up();
            return {{eye.X(), eye.Y(), eye.Z()},
                    {center.X(), center.Y(), center.Z()},
                    {up.X(), up.Y(), up.Z()},
                    camera->Scale(),
                    scale_};
        }

        void setCamera(const ModelViewerPanel::CameraState& state)
        {
            const auto& camera = view_->Camera();
            camera->SetEyeAndCenter(gp_Pnt(state.eye[0], state.eye[1], state.eye[2]),
                                    gp_Pnt(state.center[0], state.center[1], state.center[2]));
            camera->SetUp(gp_Dir(state.up[0], state.up[1], state.up[2]));
            camera->SetScale(state.scale);
            scale_ = state.zoom;
            view_->Redraw();
        }

        [[nodiscard]] std::array<double, 3> cameraEye() const
        {
            double x = 0.0;
            double y = 0.0;
            double z = 0.0;
            view_->Eye(x, y, z);
            return {x, y, z};
        }

        [[nodiscard]] int gizmoTargetIndex() const noexcept
        {
            return gizmoTargetIndex_;
        }

        // Shows a model (and the gizmo) at `transform` without changing the
        // scene: a drag on a model a task is still changing.
        void previewTransform(int modelIndex, const geometry::occt::Transform& transform)
        {
            if (!isValidIndex(modelIndex))
                return;
            context_->SetLocation(presentations_[static_cast<std::size_t>(modelIndex)].shape,
                                  TopLoc_Location(geometry::occt::toTrsf(transform)));
            if (gizmoTargetIndex_ == modelIndex)
                positionGizmoAt(transform);
            view_->Redraw();
        }

        // Puts a previewed model back where the scene has it.
        void restoreLocation(int modelIndex)
        {
            if (!isValidIndex(modelIndex))
                return;
            context_->SetLocation(presentations_[static_cast<std::size_t>(modelIndex)].shape,
                                  TopLoc_Location(geometry::occt::toTrsf(scene_.transform(modelIndex))));
            repositionGizmoIfVisible();
            view_->Redraw();
        }

        // Where the gizmo goes when its target is the selection group: its
        // centre when the selection changed, moved by the group's offset.
        void setGroupGizmo(const gp_Pnt& pivot, const geometry::occt::Transform& offset)
        {
            groupPivot_  = pivot;
            groupOffset_ = offset;
            repositionGizmoIfVisible();
            view_->Redraw();
        }

        // Centre of the models' combined bounding box in world space, each
        // placed at the given transform rather than where it is shown now.
        [[nodiscard]] gp_Pnt boundsCentre(const std::vector<std::pair<int, geometry::occt::Transform>>& placements) const
        {
            Bnd_Box all;
            for (const auto& [index, transform] : placements)
            {
                if (!isValidIndex(index))
                    continue;
                Bnd_Box box;
                presentations_[static_cast<std::size_t>(index)].shape->BoundingBox(box);
                if (box.IsVoid())
                    continue;
                const gp_Trsf shown = geometry::occt::toTrsf(scene_.transform(index));
                all.Add(box.Transformed(geometry::occt::toTrsf(transform) * shown.Inverted()));
            }
            if (all.IsVoid())
                return gp::Origin();
            double xMin = 0.0, yMin = 0.0, zMin = 0.0, xMax = 0.0, yMax = 0.0, zMax = 0.0;
            all.Get(xMin, yMin, zMin, xMax, yMax, zMax);
            return gp_Pnt((xMin + xMax) / 2.0, (yMin + yMax) / 2.0, (zMin + zMax) / 2.0);
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
        // Builds the model's MeshPickIndex on a worker thread; until it is
        // in, pick() uses its bounding box. A newer build for the same model
        // (a new mesh) wins over an older one still running.
        void buildPickIndex(application::service::ModelId id)
        {
            const auto* model = scene_.find(id);
            if (model == nullptr || !runOnUi_)
                return;
            pickIndices_.erase(id);
            const auto generation = ++pickGenerations_[id];
            platform::threading::Dispatcher::getInstance().submit(
                "viewer.pick_index", [this, id, generation, shape = model->shape, runOnUi = runOnUi_] {
                    auto index = geometry::occt::MeshPickIndex::build(shape);
                    runOnUi([this, id, generation, index = std::move(index)] {
                        if (pickGenerations_[id] == generation && scene_.find(id) != nullptr)
                            pickIndices_[id] = index;
                    });
                });
        }

        [[nodiscard]] bool isValidIndex(int modelIndex) const noexcept
        {
            return modelIndex >= 0 && static_cast<std::size_t>(modelIndex) < presentations_.size();
        }

        // Moves the gizmo to the current target's pivot - which is always
        // exactly (moveX, moveY, moveZ), since a Transform rotates each
        // model around its own local origin before translating it there.
        // World-aligned axes only for now (gp::DZ()/gp::DX()); a future
        // "local" orientation mode would build this gp_Ax2 from the
        // target's own rotation instead.
        void repositionGizmoIfVisible()
        {
            if (gizmoTool_ == ModelViewerPanel::GizmoTool::None)
                return;
            if (gizmoTargetIndex_ == ModelViewerPanel::kSelectionTarget)
                positionGizmoAt({groupPivot_.X() + groupOffset_.moveX, groupPivot_.Y() + groupOffset_.moveY,
                                 groupPivot_.Z() + groupOffset_.moveZ});
            else if (isValidIndex(gizmoTargetIndex_))
                positionGizmoAt(scene_.transform(gizmoTargetIndex_));
        }

        [[nodiscard]] bool hasGizmoTarget() const noexcept
        {
            return gizmoTool_ != ModelViewerPanel::GizmoTool::None &&
                   (gizmoTargetIndex_ >= 0 || gizmoTargetIndex_ == ModelViewerPanel::kSelectionTarget);
        }

        void positionGizmoAt(const geometry::occt::Transform& transform)
        {
            if (!hasGizmoTarget())
                return;
            manipulator_->SetPosition(
                gp_Ax2(gp_Pnt(transform.moveX, transform.moveY, transform.moveZ), gp::DZ(), gp::DX()));
        }

        void modelsChanged()
        {
            if (onModelsChanged_)
                onModelsChanged_();
        }

        application::service::SceneService& scene_;

        occ::handle<V3d_Viewer>             viewer_;
        occ::handle<V3d_View>               view_;
        occ::handle<AIS_InteractiveContext> context_;

        // What the viewport shows for one scene model.
        struct Presentation
        {
            occ::handle<AIS_Shape> shape;
            bool                   visible = true;
        };

        // One entry per scene model, at the same index.
        std::vector<Presentation> presentations_;
        double                    scale_ = 1.0;

        occ::handle<AIS_Manipulator>               manipulator_;
        occ::handle<AIS_Shape>                     manipulatorProxy_;
        int                                        gizmoTargetIndex_ = -1;
        ModelViewerPanel::GizmoTool                gizmoTool_        = ModelViewerPanel::GizmoTool::None;
        bool                                       gizmoDragging_    = false;
        geometry::occt::Transform                  dragStart_;
        gp_Pnt                                     groupPivot_;
        geometry::occt::Transform                  groupOffset_;
        std::function<void()>                      onModelsChanged_;
        std::function<bool()>                      isInteracting_;
        std::function<void(std::function<void()>)> runOnUi_;
        // Built in the background (see buildPickIndex()); null when the
        // model has no triangles; absent while building.
        std::map<application::service::ModelId, std::shared_ptr<const geometry::occt::MeshPickIndex>> pickIndices_;
        std::map<application::service::ModelId, std::uint64_t>                                        pickGenerations_;
    };

    ModelViewerPanel::ModelViewerPanel(wxWindow* parent)
        : wxPanel(parent, wxID_ANY)
        // Results come back on the UI thread; pending ones die with the panel.
        , tasks_([this](std::function<void()> callback) { CallAfter(std::move(callback)); })
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
        // The mouse-up never comes when the capture is lost (e.g. a dialog
        // pops up mid-drag): end the drag as if it did.
        canvas_->Bind(wxEVT_MOUSE_CAPTURE_LOST, [this](wxMouseCaptureLostEvent&) {
            releaseGizmo();
            rotating_      = false;
            panning_       = false;
            click_pending_ = false;
        });

        // wxBG_STYLE_PAINT must be set on the canvas itself, not just the
        // wrapping panel: it is canvas_ that OpenGL paints into. Without
        // this (and the no-op erase handler below), GTK still erases the
        // canvas to its default background colour between GL frames during
        // rapid interactive repaint (dragging to orbit/pan), which showed
        // up as the view flickering as if its brightness were changing.
        canvas_->SetBackgroundStyle(wxBG_STYLE_PAINT);
        canvas_->Bind(wxEVT_ERASE_BACKGROUND, [](wxEraseEvent&) { });
        SetBackgroundStyle(wxBG_STYLE_PAINT);

        selection_.setOnChanged([this] { onSelectionChanged(); });

        declareSettings();
        precision_subscription_ = platform::settings::SettingsRegistry::getInstance().subscribe(
            kMeshPrecisionSetting, [this](const std::string& value) { onPrecisionChanged(value); });
        brep_subscription_ = platform::settings::SettingsRegistry::getInstance().subscribe(
            kComputeBRepSetting, [this](const std::string& value) { onComputeBRepChanged(value); });
    }

    ModelViewerPanel::~ModelViewerPanel()
    {
        {
            // Background work finishing from now on drops its result.
            const std::lock_guard lock(mailbox_->mutex);
            mailbox_->panel = nullptr;
        }
        // Running work references the scene and the viewer: stop it first.
        tasks_.cancelAllAndWait();
    }

    application::tasks::TaskManager& ModelViewerPanel::tasks() noexcept
    {
        return tasks_;
    }

    void ModelViewerPanel::newFile()
    {
        try
        {
            initializeViewer();
            tasks_.submit(application::tasks::clearTask(scene_, *viewer_));
        }
        catch (const std::exception& error)
        {
            wxLogError("%s", tr("viewer.error.create", {{"error", wxString::FromUTF8(error.what())}}));
        }
    }

    void ModelViewerPanel::onPrecisionChanged(const std::string& value)
    {
        const MeshPrecision                        precision = toPrecision(value);
        std::vector<application::service::ModelId> stale;
        for (const auto& model : scene_.models())
            if (const auto current = model.shape.precision(); current && *current != precision)
                stale.push_back(model.id);
        if (stale.empty() || !initialized_)
            return;

        const wxString precisionName =
            tr(precision == MeshPrecision::Double ? "settings.import.precision_double" : "settings.import.precision_single");
        const wxString message = tr("viewer.precision.message",
                                    {{"count", wxString::Format("%zu", stale.size())}, {"precision", precisionName}});
        if (!confirmConvertLoaded(LoadedModelsQuestion::Precision, stale.size(), message))
            return;
        for (const auto id : stale)
            tasks_.submit(application::tasks::precisionTask(scene_, *viewer_, id, precision));
    }

    void ModelViewerPanel::onComputeBRepChanged(const std::string& value)
    {
        if (value != "1" || !initialized_)
            return;
        std::vector<application::service::ModelId> missing;
        for (const auto& model : scene_.models())
            if (application::tasks::needsConversion(tasks_, scene_, model.id))
                missing.push_back(model.id);
        if (missing.empty())
            return;

        const wxString message = tr("viewer.brep.message", {{"count", wxString::Format("%zu", missing.size())}});
        if (!confirmConvertLoaded(LoadedModelsQuestion::BRep, missing.size(), message))
            return;
        for (const auto id : missing)
            application::tasks::submitConversion(tasks_, scene_, *viewer_, id);
    }

    bool ModelViewerPanel::confirmConvertLoaded(LoadedModelsQuestion question, std::size_t models, const wxString& message)
    {
        if (confirm_convert_loaded_)
            return confirm_convert_loaded_(question, models);
        wxMessageDialog dialog(this, message, tr("viewer.convert_loaded.title"), wxYES_NO | wxYES_DEFAULT | wxICON_QUESTION);
        dialog.SetYesNoLabels(tr("viewer.convert_loaded.convert"), tr("viewer.convert_loaded.keep"));
        return dialog.ShowModal() == wxID_YES;
    }

    void ModelViewerPanel::openFile()
    {
        const auto path = ui::dialogs::openFile(
            this, tr("viewer.import.title"), tr("viewer.filter.stl"));
        if (!path)
            return;

        loadFile(toPath(*path));
    }

    void ModelViewerPanel::saveFile()
    {
        wxFileDialog dialog(
            this,
            tr("viewer.export.title"),
            "",
            "model.stl",
            tr("viewer.filter.stl"),
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
            const auto& settings = platform::settings::SettingsRegistry::getInstance();
            auto        task     = application::tasks::importTask(scene_, *viewer_, path,
                                                                  toPrecision(settings.get(kMeshPrecisionSetting)));
            if (settings.isOn(kComputeBRepSetting))
            {
                // Once the model is in (a failed import converts nothing).
                const auto id = task.writes.front();
                task.apply    = [this, id, apply = std::move(task.apply)] {
                    apply();
                    application::tasks::submitConversion(tasks_, scene_, *viewer_, id);
                };
            }
            tasks_.submit(std::move(task));
            return true;
        }
        catch (const std::exception& error)
        {
            wxLogError("%s", tr("viewer.error.import", {{"error", wxString::FromUTF8(error.what())}}));
            return false;
        }
    }

    bool ModelViewerPanel::exportFile(const std::filesystem::path& path)
    {
        // A model still being imported counts: the export queues behind it.
        const auto& reserved      = scene_.reservedIds();
        const bool  importPending = std::any_of(reserved.begin(), reserved.end(),
                                                [this](application::service::ModelId id) { return tasks_.wouldWait(id); });
        if (scene_.empty() && !importPending)
        {
            wxLogError("%s", tr("viewer.error.export_empty"));
            return false;
        }
        tasks_.submit(application::tasks::exportTask(scene_, path));
        return true;
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
        viewer_->setOnModelsChanged([this] { notifyModelStateChanged(); });
        mailbox_->panel = this;
        viewer_->setRunOnUi([mailbox = mailbox_](std::function<void()> callback) {
            const std::lock_guard lock(mailbox->mutex);
            if (mailbox->panel != nullptr)
                mailbox->panel->CallAfter(std::move(callback));
        });
        viewer_->setIsInteracting([this] { return rotating_ || panning_ || gizmo_dragging_; });
        showAdoptedModels();
    }

    ModelViewerPanel::ModelsSnapshot ModelViewerPanel::SnapshotModels() const
    {
        if (adopted_) // not shown yet: still what was adopted
            return *adopted_;
        return {scene_.models(), GetModelVisibility(), selection_.ids(),
                initialized_ ? std::optional(viewer_->camera()) : std::nullopt};
    }

    void ModelViewerPanel::AdoptModels(ModelsSnapshot snapshot)
    {
        snapshot.visibility.resize(snapshot.models.size(), true);
        if (!adopted_)
            adopted_ = std::move(snapshot);
        else
        {
            auto& adopted = *adopted_;
            adopted.models.insert(adopted.models.end(), std::make_move_iterator(snapshot.models.begin()),
                                  std::make_move_iterator(snapshot.models.end()));
            adopted.visibility.insert(adopted.visibility.end(), snapshot.visibility.begin(), snapshot.visibility.end());
            adopted.selected.insert(adopted.selected.end(), snapshot.selected.begin(), snapshot.selected.end());
            if (snapshot.camera)
                adopted.camera = snapshot.camera;
        }
        if (initialized_)
            showAdoptedModels();
    }

    void ModelViewerPanel::showAdoptedModels()
    {
        if (!adopted_)
            return;
        ModelsSnapshot adopted = *std::exchange(adopted_, std::nullopt);
        for (std::size_t i = 0; i < adopted.models.size(); ++i)
        {
            auto& model = adopted.models[i];
            viewer_->addModel(model.id, model.name, std::move(model.shape), model.loadDuration);
            if (model.brep)
                viewer_->setBRep(model.id, std::move(*model.brep));
            viewer_->setTransform(model.id, model.transform);
            if (!adopted.visibility[i])
                if (const auto index = scene_.indexOf(model.id))
                    viewer_->setModelVisibility(static_cast<int>(*index), false);
        }
        if (adopted.camera)
            viewer_->setCamera(*adopted.camera);
        else
            viewer_->fitAll();
        for (const auto id : adopted.selected)
            if (const auto index = scene_.indexOf(id); index && isSelectable(static_cast<int>(*index)) && !selection_.contains(id))
                selection_.toggle(id);
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

    std::vector<application::service::ModelId> ModelViewerPanel::GetModelIds() const
    {
        return scene_.ids();
    }

    std::vector<bool> ModelViewerPanel::GetModelVisibility() const
    {
        if (!initialized_ || viewer_ == nullptr)
            return {};
        return viewer_->modelVisibility();
    }

    void ModelViewerPanel::SetModelVisibility(int modelIndex, bool visible)
    {
        if (!initialized_ || viewer_ == nullptr)
            return;
        viewer_->setModelVisibility(modelIndex, visible);
        if (!visible && modelIndex >= 0 && static_cast<std::size_t>(modelIndex) < scene_.size())
            selection_.remove(scene_.models()[static_cast<std::size_t>(modelIndex)].id);
    }

    std::vector<int> ModelViewerPanel::GetSelectedIndices() const
    {
        std::vector<int> indices;
        for (const auto id : selection_.ids())
            if (const auto index = scene_.indexOf(id))
                indices.push_back(static_cast<int>(*index));
        return indices;
    }

    void ModelViewerPanel::SelectOnly(int modelIndex)
    {
        if (isSelectable(modelIndex))
            selection_.selectOnly(scene_.models()[static_cast<std::size_t>(modelIndex)].id);
    }

    void ModelViewerPanel::Select(const std::vector<int>& modelIndices)
    {
        std::vector<application::service::ModelId> ids;
        for (const int index : modelIndices)
            if (isSelectable(index))
                ids.push_back(scene_.models()[static_cast<std::size_t>(index)].id);
        selection_.set(ids);
    }

    void ModelViewerPanel::ToggleSelected(int modelIndex)
    {
        if (isSelectable(modelIndex))
            selection_.toggle(scene_.models()[static_cast<std::size_t>(modelIndex)].id);
    }

    void ModelViewerPanel::ClearSelection()
    {
        selection_.clear();
    }

    void ModelViewerPanel::SetOnSelectionChanged(std::function<void()> callback)
    {
        on_selection_changed_ = std::move(callback);
    }

    void ModelViewerPanel::SetOnBackgroundClicked(std::function<void()> callback)
    {
        on_background_clicked_ = std::move(callback);
    }

    int ModelViewerPanel::PickModelAt(const wxPoint& point) const
    {
        if (!initialized_ || viewer_ == nullptr)
            return -1;
        return viewer_->pick(point);
    }

    bool ModelViewerPanel::isSelectable(int modelIndex) const
    {
        return initialized_ && viewer_ != nullptr && modelIndex >= 0 && static_cast<std::size_t>(modelIndex) < scene_.size();
    }

    void ModelViewerPanel::handleClick(const wxPoint& point, bool toggle)
    {
        const int picked = PickModelAt(point);
        if (picked >= 0)
        {
            if (toggle)
                ToggleSelected(picked);
            else
                SelectOnly(picked);
        }
        else if (!toggle)
        {
            ClearSelection();
            if (on_background_clicked_)
                on_background_clicked_();
        }
        if (initialized_)
            viewer_->paint();
    }

    void ModelViewerPanel::onSelectionChanged()
    {
        resetGroup();
        if (initialized_ && viewer_ != nullptr)
        {
            for (std::size_t i = 0; i < scene_.size(); ++i)
                viewer_->setHighlighted(static_cast<int>(i), selection_.contains(scene_.models()[i].id));
            viewer_->paint();
        }
        if (on_selection_changed_)
            on_selection_changed_();
    }

    std::vector<int> ModelViewerPanel::DuplicateModels(const std::vector<int>& modelIndices, int copies)
    {
        std::vector<int> added;
        if (!initialized_ || viewer_ == nullptr)
            return added;
        // By id: adding a model may move the others in memory.
        std::vector<application::service::ModelId> sources;
        for (const int index : modelIndices)
            if (index >= 0 && static_cast<std::size_t>(index) < scene_.size())
                sources.push_back(scene_.models()[static_cast<std::size_t>(index)].id);
        for (int copy = 0; copy < copies; ++copy)
            for (const auto id : sources)
            {
                const auto source = scene_.find(id);
                if (source == nullptr)
                    continue;
                const auto sourceIndex = *scene_.indexOf(id);
                const bool visible     = viewer_->modelVisibility()[sourceIndex];
                const auto model       = *source; // shapes are shared handles: cheap
                viewer_->addModel(0, application::service::SceneService::baseName(model.name), model.shape, model.loadDuration);
                const auto copyId = scene_.models().back().id;
                if (model.brep)
                    viewer_->setBRep(copyId, *model.brep);
                viewer_->setTransform(copyId, model.transform);
                const int index = static_cast<int>(scene_.size() - 1);
                if (!visible)
                    viewer_->setModelVisibility(index, false);
                added.push_back(index);
            }
        return added;
    }

    void ModelViewerPanel::RemoveModel(int modelIndex)
    {
        if (!initialized_ || viewer_ == nullptr)
            return;
        if (modelIndex < 0 || static_cast<std::size_t>(modelIndex) >= scene_.size())
        {
            notifyModelStateChanged(); // nothing to remove; listeners still resync
            return;
        }
        tasks_.submit(application::tasks::removeTask(
            scene_, *viewer_, scene_.models()[static_cast<std::size_t>(modelIndex)].id));
    }

    void ModelViewerPanel::FocusModel(int modelIndex)
    {
        if (!initialized_ || viewer_ == nullptr)
            return;
        viewer_->focusModel(modelIndex);
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

        const geometry::occt::Transform transform{moveXmm, moveYmm, moveZmm, rotateXdeg, rotateYdeg, rotateZdeg};
        if (targetIndex == kSelectionTarget)
        {
            if (!isGroup())
                return;
            group_.offset = transform;
            const gp_Pnt pivot(group_.pivot[0], group_.pivot[1], group_.pivot[2]);
            viewer_->setGroupGizmo(pivot, transform);
            // Always from where the models were when the selection changed, so
            // edits never pile up rounding errors.
            for (const auto& [id, start] : group_.start)
                submitMove(id, geometry::occt::placeInGroup(start, pivot, transform));
            return;
        }

        std::vector<application::service::ModelId> targets;
        if (targetIndex == application::service::SceneService::kAllModels)
            targets = scene_.ids();
        else if (targetIndex >= 0 && static_cast<std::size_t>(targetIndex) < scene_.size())
            targets.push_back(scene_.models()[static_cast<std::size_t>(targetIndex)].id);

        // Each applies at once, or queues behind a task still changing that model.
        for (const auto id : targets)
            submitMove(id, transform);
    }

    ModelViewerPanel::TransformValues ModelViewerPanel::GetTransform(int targetIndex) const
    {
        if (targetIndex == kSelectionTarget)
            return isGroup() ? group_.offset : TransformValues{};
        // While a drag on a busy model is previewed, show what is previewed.
        if (targetIndex >= 0 && static_cast<std::size_t>(targetIndex) < scene_.size())
            if (const auto pending = pending_drags_.find(scene_.models()[static_cast<std::size_t>(targetIndex)].id);
                pending != pending_drags_.end())
                return pending->second;
        return scene_.transform(targetIndex);
    }

    void ModelViewerPanel::SetGizmoState(int targetIndex, GizmoTool tool)
    {
        if (!initialized_ || viewer_ == nullptr)
            return;

        if (targetIndex == kSelectionTarget && !isGroup())
            targetIndex = application::service::SceneService::kAllModels; // nothing to show
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
        // Models first: selection listeners then see the current list.
        if (on_model_state_changed_)
            on_model_state_changed_(hasModel());
        selection_.retainOnly(scene_.ids());
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
            wxLogError("%s", tr("viewer.error.init", {{"error", wxString::FromUTF8(error.what())}}));
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
        // Keys go to the viewport now (e.g. Delete), not a field typed in before.
        canvas_->SetFocus();
        if (event.LeftDown() && initialized_)
        {
            if (viewer_->startGizmoDragIfHit(last_mouse_position_))
            {
                gizmo_dragging_ = true;
                canvas_->CaptureMouse();
                return;
            }

            press_position_ = last_mouse_position_;
            click_pending_  = true;
            rotating_       = true;
            viewer_->startRotation(last_mouse_position_);
        }
        if (event.MiddleDown())
            panning_ = true;
        canvas_->CaptureMouse();
    }

    void ModelViewerPanel::dragGizmoTo(int modelIndex, const geometry::occt::Transform& transform)
    {
        if (modelIndex == kSelectionTarget)
        {
            if (!isGroup())
                return;
            group_.offset = transform;
            const gp_Pnt pivot(group_.pivot[0], group_.pivot[1], group_.pivot[2]);
            viewer_->setGroupGizmo(pivot, transform);
            for (const auto& [id, start] : group_.start)
                dragModelTo(id, geometry::occt::placeInGroup(start, pivot, transform));
            return;
        }
        if (modelIndex < 0 || static_cast<std::size_t>(modelIndex) >= scene_.size())
            return;
        dragModelTo(scene_.models()[static_cast<std::size_t>(modelIndex)].id, transform);
    }

    void ModelViewerPanel::dragModelTo(application::service::ModelId id, const geometry::occt::Transform& transform)
    {
        if (tasks_.wouldWait(id))
        {
            if (const auto index = scene_.indexOf(id))
                viewer_->previewTransform(static_cast<int>(*index), transform);
            pending_drags_[id] = transform;
        }
        else
        {
            pending_drags_.erase(id);
            submitMove(id, transform);
        }
    }

    void ModelViewerPanel::submitMove(application::service::ModelId id, const geometry::occt::Transform& transform)
    {
        queued_moves_[id] = transform;
        tasks_.submit(application::tasks::moveTask(scene_, *viewer_, id, transform));
    }

    geometry::occt::Transform ModelViewerPanel::latestTransform(application::service::ModelId id) const
    {
        if (const auto pending = pending_drags_.find(id); pending != pending_drags_.end())
            return pending->second;
        if (const auto queued = queued_moves_.find(id); queued != queued_moves_.end() && tasks_.wouldWait(id))
            return queued->second;
        const auto* model = scene_.find(id);
        return model != nullptr ? model->transform : geometry::occt::Transform{};
    }

    void ModelViewerPanel::resetGroup()
    {
        group_ = {};
        if (!initialized_ || viewer_ == nullptr || selection_.size() < 2)
        {
            if (initialized_ && viewer_ != nullptr && viewer_->gizmoTargetIndex() == kSelectionTarget)
                viewer_->setGizmoTarget(application::service::SceneService::kAllModels, GizmoTool::None);
            return;
        }
        // From where the models are headed, so a move still queued isn't undone.
        std::vector<std::pair<int, geometry::occt::Transform>> placements;
        for (const int index : GetSelectedIndices())
        {
            const auto id        = scene_.models()[static_cast<std::size_t>(index)].id;
            const auto transform = latestTransform(id);
            group_.start.emplace_back(id, transform);
            placements.emplace_back(index, transform);
        }
        const gp_Pnt pivot = viewer_->boundsCentre(placements);
        group_.pivot       = {pivot.X(), pivot.Y(), pivot.Z()};
        viewer_->setGroupGizmo(pivot, group_.offset);
    }

    bool ModelViewerPanel::isGroup() const noexcept
    {
        return selection_.size() >= 2 && !group_.start.empty();
    }

    void ModelViewerPanel::releaseGizmo()
    {
        if (gizmo_dragging_ && initialized_)
        {
            viewer_->stopGizmoDrag();
            // A model was busy: what was previewed becomes a queued move.
            auto pending = std::move(pending_drags_);
            pending_drags_.clear();
            for (const auto& [id, transform] : pending)
            {
                if (const auto index = scene_.indexOf(id))
                    viewer_->restoreLocation(static_cast<int>(*index));
                submitMove(id, transform);
            }
        }
        gizmo_dragging_ = false;
    }

#if defined(SPLINTER3D_TESTING)
    void ModelViewerPanel::SimulateGizmoDrag(int modelIndex, const TransformValues& transform)
    {
        if (!initialized_)
            return;
        gizmo_dragging_ = true;
        dragGizmoTo(modelIndex, transform);
        if (on_gizmo_changed_)
            on_gizmo_changed_();
    }

    void ModelViewerPanel::SimulateGizmoRelease()
    {
        releaseGizmo();
    }

    std::array<double, 3> ModelViewerPanel::CameraEye() const
    {
        return initialized_ ? viewer_->cameraEye() : std::array<double, 3>{};
    }

    wxPoint ModelViewerPanel::ProjectToCanvas(const gp_Pnt& point) const
    {
        return initialized_ ? viewer_->project(point) : wxPoint();
    }

    void ModelViewerPanel::FitAll()
    {
        if (initialized_)
            viewer_->fitAll();
    }

    void ModelViewerPanel::SimulateClick(const wxPoint& point, bool toggle)
    {
        if (!initialized_)
            return;
        // As onMouseDown()/onMouseUp(): a press on the gizmo is a drag.
        if (viewer_->startGizmoDragIfHit(point))
        {
            gizmo_dragging_ = true;
            releaseGizmo();
            return;
        }
        handleClick(point, toggle);
    }

    bool ModelViewerPanel::IsHighlighted(int modelIndex) const
    {
        return initialized_ && viewer_->isHighlighted(modelIndex);
    }

    bool ModelViewerPanel::PickDataReady() const
    {
        return initialized_ && viewer_->pickIndicesReady();
    }

    bool ModelViewerPanel::HasHoverHighlight() const
    {
        return initialized_ && viewer_->hasDetected();
    }

    void ModelViewerPanel::SetConfirmConvertLoaded(
        std::function<bool(LoadedModelsQuestion question, std::size_t models)> confirm)
    {
        confirm_convert_loaded_ = std::move(confirm);
    }

    const application::service::SceneService& ModelViewerPanel::Scene() const noexcept
    {
        return scene_;
    }
#endif

    void ModelViewerPanel::onMouseUp(wxMouseEvent& event)
    {
        if (event.LeftUp())
        {
            releaseGizmo();
            rotating_ = false;
            // Motion events may not come in between (a fast flick): check the release too.
            const wxPoint release = event.GetPosition();
            if (click_pending_ && std::abs(release.x - press_position_.x) <= kClickSlop &&
                std::abs(release.y - press_position_.y) <= kClickSlop)
                handleClick(release, event.ControlDown()); // Cmd on macOS
            click_pending_ = false;
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
            // Both paths redraw; no need to redraw again here.
            if (const auto transform = viewer_->gizmoDragTransform(position))
                dragGizmoTo(viewer_->gizmoTargetIndex(), *transform);
            last_mouse_position_ = position;
            if (on_gizmo_changed_)
                on_gizmo_changed_();
            return;
        }

        if (!rotating_ && !panning_)
            return;

        // Until the pointer leaves the click zone, this may still be a click:
        // don't nudge the camera.
        if (click_pending_)
        {
            if (std::abs(position.x - press_position_.x) <= kClickSlop &&
                std::abs(position.y - press_position_.y) <= kClickSlop)
                return;
            click_pending_ = false;
        }

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
        if (!initialized_ || event.GetWheelAxis() != wxMOUSE_WHEEL_VERTICAL || event.GetWheelRotation() == 0)
            return;
        const int delta = event.GetWheelDelta() > 0 ? event.GetWheelDelta() : 120;
        viewer_->zoom(static_cast<double>(event.GetWheelRotation()) / delta);
        viewer_->paint();
    }
} // namespace ui::framework::occt
