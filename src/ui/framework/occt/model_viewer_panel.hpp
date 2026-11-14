#pragma once

#include "application/service/scene_service.hpp"
#include "application/service/selection.hpp"
#include "application/tasks/task_manager.hpp"
#include "geometry/occt/transform.hpp"
#include "platform/settings/settings_registry.hpp"

#include <array>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <utility>
#include <vector>
#include <wx/glcanvas.h>
#include <wx/panel.h>
#include <wx/string.h>

class gp_Pnt;

namespace ui::framework::occt
{
    // Settings the viewer declares (see platform::settings::SettingsRegistry).
    inline constexpr const char* kMeshPrecisionSetting = "import.mesh_precision"; // "single" or "double"
    inline constexpr const char* kComputeBRepSetting   = "import.compute_brep";   // convert each import to a B-Rep

    class ModelViewerPanel final : public wxPanel
    {
      public:
        // Move (millimeters) and rotate (degrees, around the origin) values
        // currently applied to a given target. See setTransform().
        using TransformValues = geometry::occt::Transform;

        // Target index meaning "the selected models, as one group" (with at
        // least two selected). Its transform is an offset, all zero when the
        // selection changes: the group turns by its angles (X, then Y, then
        // Z) around the centre of the models' combined bounding box, taken
        // when the selection changed, then moves by its move. See
        // geometry::occt::placeInGroup().
        static constexpr int kSelectionTarget = -2;

        explicit ModelViewerPanel(wxWindow* parent);
        ~ModelViewerPanel() override;

        void newFile();
        // Asks for an STL file with a dialog, then loadFile()s it.
        void openFile();
        // Asks for a destination with a dialog, then exportFile()s to it.
        void saveFile();

        // Starts importing an STL file in the background (a task); the model
        // appears once it is read. Returns false only if the viewer could not
        // start; import errors show on the task's card. Its mesh precision and
        // whether a B-Rep conversion follows come from the settings.
        bool loadFile(const std::filesystem::path& path);

        // Starts writing the scene to an STL file in the background (a task).
        // Returns false (and logs) if no model is loaded or being imported;
        // errors show on the task's card.
        bool exportFile(const std::filesystem::path& path);

        // The background tasks working on this panel's scene (see TasksPanel).
        [[nodiscard]] application::tasks::TaskManager& tasks() noexcept;

        // True as soon as a shape is loaded in the viewer.
        [[nodiscard]] bool hasModel() const;

        // Where the camera looks from and at, which way is up, and how far
        // it is zoomed (the view's scale, and the wheel's zoom factor).
        struct CameraState
        {
            std::array<double, 3> eye{};
            std::array<double, 3> center{};
            std::array<double, 3> up{};
            double                scale = 1.0;
            double                zoom  = 1.0;
        };

        // The open models (ids, names, shapes, placements, B-Reps), whether
        // each is shown, the selection and the camera, to carry over to
        // another viewer.
        struct ModelsSnapshot
        {
            std::vector<application::service::SceneService::Model> models;
            std::vector<bool>                                      visibility;
            std::vector<application::service::ModelId>             selected; // in the order picked
            std::optional<CameraState>                             camera;   // none before the viewport exists
        };

        // Copies the open models (shapes are shared, not duplicated).
        // Meant for when no task is pending: a model still being imported
        // is not in it.
        [[nodiscard]] ModelsSnapshot SnapshotModels() const;

        // Adds a snapshot's models with their ids, names, placements,
        // B-Reps and visibility, selects what it had selected, and puts the
        // camera where it was (else frames every model). Waits for the
        // viewport to exist (its first paint) if it doesn't yet.
        void AdoptModels(ModelsSnapshot snapshot);

        // Display names of every currently loaded model, in load order.
        // Index i matches setTransform()/GetTransform()'s targetIndex.
        [[nodiscard]] std::vector<wxString> GetModelNames() const;

        // Stable ids of every loaded model, same order as GetModelNames().
        [[nodiscard]] std::vector<application::service::ModelId> GetModelIds() const;

        [[nodiscard]] std::vector<bool> GetModelVisibility() const;
        void                            SetModelVisibility(int modelIndex, bool visible);
        void                            RemoveModel(int modelIndex);
        void                            FocusModel(int modelIndex);

        // The models the user picked, drawn in a lighter shade: a click on a
        // model selects just it, Cmd/Ctrl-click adds or removes it, a click
        // on empty space clears the selection. Indices as GetModelNames(), in
        // the order they were picked. Hidden models can't be selected, and
        // leave the selection when hidden or removed.
        [[nodiscard]] std::vector<int> GetSelectedIndices() const;
        void                           SelectOnly(int modelIndex);
        void                           ToggleSelected(int modelIndex);
        void                           ClearSelection();
        // Invoked after every change of the selection.
        void SetOnSelectionChanged(std::function<void()> callback);
        // Invoked when a plain click on empty space cleared the selection.
        void SetOnBackgroundClicked(std::function<void()> callback);
        // The visible model under a canvas point (the nearest), or -1.
        [[nodiscard]] int PickModelAt(const wxPoint& point) const;

        // Moves (millimeters) and rotates (degrees, around the origin) a
        // model, replacing whatever transform was previously applied to it.
        // targetIndex selects which one: -1 applies the same absolute
        // transform to every loaded model, otherwise it is an index into
        // GetModelNames(). Does nothing if no model is loaded or the index
        // is out of range. Applies at once, unless a task is still changing
        // that model: then it is queued behind it (FIFO).
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

        // What changing an import setting asks about the models already loaded.
        enum class LoadedModelsQuestion
        {
            Precision, // store their meshes in the new precision too?
            BRep,      // compute their B-Reps too?
        };

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
        // hides the gizmo since there is no single meaningful pivot for it;
        // kSelectionTarget shows it at the group's centre.
        void SetGizmoState(int targetIndex, GizmoTool tool);

        // Invoked whenever a gizmo drag changes a model's transform, so
        // callers (e.g. the numeric fields) can refresh from GetTransform().
        void SetOnGizmoChanged(std::function<void()> callback);

        // Invoked with hasModel() every time a model is loaded or cleared,
        // so other widgets (e.g. the transform toolbar) can react.
        void SetOnModelStateChanged(std::function<void(bool)> callback);

#if defined(SPLINTER3D_TESTING)
        // Test-only: what a gizmo drag to `transform` on model `modelIndex`,
        // then the mouse button's release, do (a real drag needs the mouse
        // on a gizmo handle, which tests can't aim at).
        void SimulateGizmoDrag(int modelIndex, const TransformValues& transform);
        void SimulateGizmoRelease();

        // Test-only: where the camera is.
        [[nodiscard]] std::array<double, 3> CameraEye() const;
        // Test-only: where a world point shows on the canvas.
        [[nodiscard]] wxPoint ProjectToCanvas(const gp_Pnt& point) const;
        // Test-only: frames every displayed model.
        void FitAll();
        // Test-only: what a left click (press and release in place) at
        // point does; toggle = Cmd/Ctrl held.
        void SimulateClick(const wxPoint& point, bool toggle);
        // Test-only: whether a model is drawn as selected.
        [[nodiscard]] bool IsHighlighted(int modelIndex) const;
        // Test-only: whether every model's click data (built in the
        // background after each load or mesh change) is in.
        [[nodiscard]] bool PickDataReady() const;
        // Test-only: whether OCCT is hover-highlighting something.
        [[nodiscard]] bool HasHoverHighlight() const;

        // Test-only: replaces the "convert loaded models too?" dialog shown
        // when an import setting changes (true = convert).
        void SetConfirmConvertLoaded(std::function<bool(LoadedModelsQuestion question, std::size_t models)> confirm);

        // Test-only: the models as the scene holds them.
        [[nodiscard]] const application::service::SceneService& Scene() const noexcept;
#endif

      private:
        void notifyModelStateChanged();

        // A click (not a drag) at point; toggle = Cmd/Ctrl held.
        void handleClick(const wxPoint& point, bool toggle);
        // Redraws the highlights and tells on_selection_changed_.
        void               onSelectionChanged();
        [[nodiscard]] bool isSelectable(int modelIndex) const;

        // The mesh precision setting changed: offers to convert the loaded
        // meshes stored the other way.
        void onPrecisionChanged(const std::string& value);
        // B-Reps on import turned on: offers to convert the loaded models
        // that have none.
        void onComputeBRepChanged(const std::string& value);
        // Asks whether to apply an import setting to `models` loaded models.
        bool confirmConvertLoaded(LoadedModelsQuestion question, std::size_t models, const wxString& message);

        // Commits a gizmo drag position (kSelectionTarget: the group's
        // offset), or previews it while a task still changes the model (see
        // pending_drags_).
        void dragGizmoTo(int modelIndex, const geometry::occt::Transform& transform);
        void dragModelTo(application::service::ModelId id, const geometry::occt::Transform& transform);
        // Queues a move (see queued_moves_).
        void submitMove(application::service::ModelId id, const geometry::occt::Transform& transform);
        // Where a model will be once what is queued on it ran: a previewed
        // drag, else its last move still waiting, else its placement now.
        [[nodiscard]] geometry::occt::Transform latestTransform(application::service::ModelId id) const;
        // The selected models' placements and centre, taken anew (offset back
        // to zero) on every selection change.
        void               resetGroup();
        [[nodiscard]] bool isGroup() const noexcept;
        // Ends a gizmo drag: a previewed position becomes a queued move.
        void releaseGizmo();

        void onPaint(wxPaintEvent& event);
        void onSize(wxSizeEvent& event);
        void onMouseDown(wxMouseEvent& event);
        void onMouseUp(wxMouseEvent& event);
        void onMouseMove(wxMouseEvent& event);
        void onMouseWheel(wxMouseEvent& event);
        void initializeViewer();
        void showAdoptedModels();

        // Declared before viewer_, which holds a reference to it.
        application::service::SceneService scene_;
        std::unique_ptr<wxGLContext>       context_;
        wxGLCanvas*                        canvas_ = nullptr;
        class Viewer;
        std::unique_ptr<Viewer> viewer_;
        bool                    initialized_    = false;
        bool                    rotating_       = false;
        bool                    panning_        = false;
        bool                    gizmo_dragging_ = false;
        wxPoint                 last_mouse_position_;
        // Where the left button went down, and whether its release would
        // still be a click (it hasn't moved further than a few pixels).
        wxPoint                         press_position_;
        bool                            click_pending_ = false;
        std::function<void(bool)>       on_model_state_changed_;
        std::function<void()>           on_gizmo_changed_;
        std::function<void()>           on_selection_changed_;
        std::function<void()>           on_background_clicked_;
        application::service::Selection selection_;
        // Last gizmo drag position on each model a task is still changing:
        // only previewed, queued as a move on mouse up.
        std::map<application::service::ModelId, geometry::occt::Transform> pending_drags_;
        // Last move submitted for each model; it may still wait behind a task.
        std::map<application::service::ModelId, geometry::occt::Transform> queued_moves_;

        // The group kSelectionTarget moves: where its models were and its
        // centre when the selection changed, and the offset applied since.
        struct Group
        {
            std::vector<std::pair<application::service::ModelId, geometry::occt::Transform>> start;
            std::array<double, 3>                                                            pivot{};
            geometry::occt::Transform                                                        offset;
        };

        Group                                                         group_;
        std::optional<ModelsSnapshot>                                 adopted_;
        std::function<bool(LoadedModelsQuestion, std::size_t models)> confirm_convert_loaded_;
        platform::settings::Subscription                              precision_subscription_;
        platform::settings::Subscription                              brep_subscription_;

        // Lets background work post back to this panel only while it exists.
        struct Mailbox
        {
            std::mutex        mutex;
            ModelViewerPanel* panel = nullptr;
        };

        std::shared_ptr<Mailbox> mailbox_ = std::make_shared<Mailbox>();
        // Declared last, destroyed first: its tasks reference scene_ and viewer_.
        application::tasks::TaskManager tasks_;
    };
} // namespace ui::framework::occt
