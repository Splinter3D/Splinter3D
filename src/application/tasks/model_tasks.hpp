#pragma once

#include "application/service/scene_service.hpp"
#include "scene_editor.hpp"
#include "task.hpp"

#include <filesystem>

namespace application::tasks
{
    /**
     * @brief Tasks for the operations on models. Each reads the scene when it
     * is created (on the UI thread) to know which models it touches, and
     * commits through editor in apply(). The scene and editor must outlive
     * the task.
     */

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
} // namespace application::tasks
