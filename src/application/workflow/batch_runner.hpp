#pragma once

#include "application/service/scene_service.hpp"

#include <cstddef>
#include <filesystem>
#include <ostream>
#include <string>
#include <string_view>

namespace application::workflow
{
    /**
     * @brief Runs a scripted, non-interactive scenario against a SceneService.
     *
     * Used by `splinter3D --batch <script.json>` (see app/cli/batch_mode.hpp)
     * and by the end-to-end tests. A script is a JSON object whose "steps"
     * array is executed in order, stopping at the first failure:
     *
     * @code{.json}
     * {
     *   "steps": [
     *     { "open": "models/cube.stl" },
     *     { "transform": { "target": 0, "move": [10, 0, 0], "rotate": [0, 0, 90] } },
     *     { "export": "out/cube.stl" },
     *     { "clear": true }
     *   ]
     * }
     * @endcode
     *
     * - "open": imports an STL file (like File > Open).
     * - "transform": sets a model's absolute placement, like the toolbar fields.
     *   "target" is a model index or "all" (default); "move" is in millimeters,
     *   "rotate" in degrees; omitted vectors are [0, 0, 0].
     * - "export": writes the scene to an STL file (like File > Export).
     * - "clear": removes every model (like File > New).
     *
     * Relative paths are resolved against the script's directory.
     */
    class BatchRunner
    {
      public:
        struct Result
        {
            bool        ok       = false;
            std::size_t stepsRun = 0; ///< Steps that completed successfully.
            std::string error;        ///< Why the run stopped, when !ok.
        };

        /**
         * @param scene Scene the steps operate on.
         * @param log Receives one line per step.
         */
        BatchRunner(service::SceneService& scene, std::ostream& log);

        /**
         * @brief Runs a script given as JSON text.
         *
         * @param baseDir Directory relative paths are resolved against.
         */
        Result run(std::string_view scriptJson, const std::filesystem::path& baseDir);

        /**
         * @brief Reads and runs a script file; relative paths in it are
         * resolved against the file's own directory.
         */
        Result runFile(const std::filesystem::path& scriptPath);

      private:
        service::SceneService& _scene;
        std::ostream&          _log;
    };
} // namespace application::workflow
