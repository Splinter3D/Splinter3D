#include "batch_mode.hpp"

#include "application/service/scene_service.hpp"
#include "application/workflow/batch_runner.hpp"
#include "core/utils/data_root.hpp"
#include "core/utils/user_dirs.hpp"
#include "platform/locale/locale_manager.hpp"
#include "platform/threading/dispatcher.hpp"

#include <algorithm>
#include <filesystem>
#include <iostream>

namespace app::cli
{
    namespace
    {
        std::filesystem::path dataRootOf(const std::string& program)
        {
            std::error_code             ec;
            const std::filesystem::path executable =
                std::filesystem::absolute(std::filesystem::path(std::u8string(program.begin(), program.end())), ec);
            std::filesystem::path dataRoot = core::utils::findDataRoot(ec ? std::filesystem::path() : executable);
            if (dataRoot.empty())
                dataRoot = std::filesystem::current_path();
            return dataRoot;
        }
    } // namespace

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
        auto&                              dispatcher = platform::threading::Dispatcher::getInstance();
        const auto                         dataRoot   = dataRootOf(args.front());
        // Scripts and their output are machine-read: keep the messages in English.
        platform::locale::LocaleManager::get().init((dataRoot / "assets" / "locale").string());
        platform::locale::LocaleManager::get().setLanguage("en");
        dispatcher.load(platform::threading::Dispatcher::locateProfiles(core::utils::profilesDir(dataRoot), dataRoot));
        const auto result = runner.runFile(script);
        dispatcher.save();
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
