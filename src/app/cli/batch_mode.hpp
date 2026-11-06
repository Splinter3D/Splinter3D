#pragma once

#include <optional>
#include <string>
#include <vector>

namespace app::cli
{
    /// Exit codes of `splinter3D --batch`.
    enum BatchExitCode : int
    {
        kBatchOk         = 0, ///< Every step succeeded.
        kBatchStepFailed = 1, ///< A step failed (missing model, export error...).
        kBatchUsageError = 2, ///< Bad command line, unreadable or invalid script.
    };

    /**
     * @brief Runs `--batch <script.json>` without starting the GUI, if requested.
     *
     * Called from main() before wxWidgets is initialized, so batch mode needs
     * no display (headless CI, servers). See application::workflow::BatchRunner
     * for the script format.
     *
     * @param args The command line, UTF-8, args[0] being the program.
     * @return The process exit code if batch mode ran, std::nullopt to start
     * the GUI as usual.
     */
    std::optional<int> runBatchIfRequested(const std::vector<std::string>& args);
} // namespace app::cli
