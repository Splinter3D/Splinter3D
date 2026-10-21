#include "batch_mode.hpp"

#include "application/service/scene_service.hpp"
#include "application/workflow/batch_runner.hpp"

#include <algorithm>
#include <filesystem>
#include <iostream>

namespace app::cli
{
    std::optional<int> runBatchIfRequested(const std::vector<std::string>& args)
    {
        const auto flag = std::find(args.begin(), args.end(), "--batch");
        if (flag == args.end())
            return std::nullopt;

        if (std::next(flag) == args.end())
        {
            std::cerr << "usage: splinter3D --batch <script.json>\n";
            return kBatchUsageError;
        }

        // args are UTF-8 (see main.cpp), which std::filesystem::path(std::string)
        // would misread on Windows.
        const std::string&          scriptArg = *std::next(flag);
        const std::filesystem::path script(std::u8string(scriptArg.begin(), scriptArg.end()));
        if (!std::filesystem::is_regular_file(script))
        {
            std::cerr << "[batch] script not found: " << scriptArg << "\n";
            return kBatchUsageError;
        }

        application::service::SceneService scene;
        application::workflow::BatchRunner runner(scene, std::cout);
        const auto                         result = runner.runFile(script);
        std::cout.flush();

        if (result.ok)
        {
            std::cout << "[batch] done: " << result.stepsRun << " step(s)\n";
            return kBatchOk;
        }

        std::cerr << "[batch] failed: " << result.error << "\n";
        // Nothing ran at all: the script itself is unusable.
        return result.stepsRun == 0 && result.error.rfind("step ", 0) != 0 ? kBatchUsageError : kBatchStepFailed;
    }
} // namespace app::cli
