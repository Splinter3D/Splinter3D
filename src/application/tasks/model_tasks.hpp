#pragma once

#include "application/service/scene_service.hpp"
#include "scene_editor.hpp"
#include "task.hpp"
#include "task_manager.hpp"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace application::tasks
{
    /**
     * @brief Tasks for the operations on models. Each reads the scene when it
     * is created (on the UI thread) to know which models it touches, and
     * commits through editor in apply(). The scene and editor must outlive
     * the task.
     */

    /** @brief From this file size on, an import has its own Dispatcher profile (see importOperation()). */
    inline constexpr std::uintmax_t kLargeImportBytes = std::uintmax_t{64} << 20;

    /**
     * @brief The Dispatcher operation key of importing a file of `bytes`:
     * small files barely use a second core, so sharing their profile would
     * teach the Dispatcher that big imports don't scale either.
     */
    [[nodiscard]] std::string importOperation(std::uintmax_t bytes);

    /** @brief "Import <file>": reads the STL in the background, then adds it under an id reserved now. */
    [[nodiscard]] Task importTask(service::SceneService& scene, SceneEditor& editor, std::filesystem::path path);

    /**
     * @brief "Export <file>": every model, including ones still being
     * imported by earlier tasks, as they are once earlier changes applied.
     */
    [[nodiscard]] Task exportTask(const service::SceneService& scene, std::filesystem::path path);

    /** @brief "Move <model>": sets the model's transform. */
    [[nodiscard]] Task moveTask(const service::SceneService& scene,
                                SceneEditor&                 editor,
                                service::ModelId             id,
                                geometry::occt::Transform    transform);

    /** @brief "Remove <model>". */
    [[nodiscard]] Task removeTask(const service::SceneService& scene, SceneEditor& editor, service::ModelId id);

    /** @brief "Clear scene": every model, including ones still being imported. */
    [[nodiscard]] Task clearTask(const service::SceneService& scene, SceneEditor& editor);

    /**
     * @brief The task resource standing for a model's B-Rep cache: a
     * conversion writes it, operations that need the B-Rep read it. A
     * conversion only reads the model itself, so moving, removing or
     * exporting it never waits for a (possibly minutes-long) conversion.
     */
    [[nodiscard]] constexpr service::ModelId brepResource(service::ModelId id) noexcept
    {
        return id | (service::ModelId{1} << 63);
    }

    /**
     * @brief "Convert <model>": builds the model's B-Rep in the background
     * (one face per triangle; slow on big meshes) and caches it; a model
     * that already is a B-Rep is cached as is. Cancelling leaves the model a
     * mesh; a model removed meanwhile drops the result.
     */
    [[nodiscard]] Task convertTask(const service::SceneService& scene, SceneEditor& editor, service::ModelId id);

    /**
     * @brief For an operation's prepare(): fails with "model "<name>" could not
     * be converted" when the model has no B-Rep (its conversion was cancelled
     * or failed), or "... no longer exists" when it is gone.
     */
    [[nodiscard]] std::function<std::optional<std::string>()> requireBRep(const service::SceneService& scene,
                                                                          service::ModelId             id);

    /**
     * @brief Submits operation (which needs the B-Rep of `models`), after a
     * conversion for each model that has neither a B-Rep nor a pending
     * conversion. FIFO per model makes the conversions apply first.
     *
     * @return The ids submitted: the conversions, then the operation.
     */
    std::vector<TaskId> submitWithBRep(TaskManager&                  tasks,
                                       const service::SceneService&  scene,
                                       SceneEditor&                  editor,
                                       std::vector<service::ModelId> models,
                                       Task                          operation);
} // namespace application::tasks
