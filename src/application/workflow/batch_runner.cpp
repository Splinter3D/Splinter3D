#include "batch_runner.hpp"

#include "application/tasks/model_tasks.hpp"
#include "geometry/occt/path.hpp"

#include <array>
#include <fstream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>

namespace application::workflow
{
    namespace
    {
        using json = nlohmann::json;

        // JSON text is UTF-8, but std::filesystem::path(std::string) assumes the
        // native narrow encoding (the ANSI code page on Windows).
        std::filesystem::path pathFromUtf8(const std::string& utf8)
        {
            return std::filesystem::path(std::u8string(utf8.begin(), utf8.end()));
        }

        // A step failed for a reason worth reporting as-is (bad arguments, I/O...).
        struct StepError : std::runtime_error
        {
            using std::runtime_error::runtime_error;
        };

        std::filesystem::path resolve(const json& value, const std::filesystem::path& baseDir)
        {
            if (!value.is_string() || value.get<std::string>().empty())
                throw StepError("expected a non-empty path string");
            const std::filesystem::path path = pathFromUtf8(value.get<std::string>());
            return path.is_absolute() ? path : baseDir / path;
        }

        std::array<double, 3> vector3(const json& args, const char* key)
        {
            if (!args.contains(key))
                return {0.0, 0.0, 0.0};
            const json& value = args.at(key);
            if (!value.is_array() || value.size() != 3 ||
                !std::all_of(value.begin(), value.end(), [](const json& v) { return v.is_number(); }))
                throw StepError(std::string("\"") + key + "\" must be an array of 3 numbers");
            return {value[0].get<double>(), value[1].get<double>(), value[2].get<double>()};
        }

        int target(const json& args)
        {
            if (!args.contains("target"))
                return service::SceneService::kAllModels;
            const json& value = args.at("target");
            if (value == "all")
                return service::SceneService::kAllModels;
            if (value.is_number_integer() && value.get<long long>() >= 0)
                return static_cast<int>(value.get<long long>());
            throw StepError("\"target\" must be a model index or \"all\"");
        }
    } // namespace

    BatchRunner::BatchRunner(service::SceneService& scene, std::ostream& log)
        : _scene(scene)
        , _log(log)
        , _editor(scene)
    { }

    void BatchRunner::_run(tasks::Task task)
    {
        const tasks::TaskId id       = _tasks.submit(std::move(task));
        const auto          snapshot = _tasks.snapshot(id);
        if (!snapshot || snapshot->state != tasks::TaskState::Failed)
            return;
        _tasks.dismiss(id);
        throw StepError(snapshot->error);
    }

    BatchRunner::Result BatchRunner::runFile(const std::filesystem::path& scriptPath)
    {
        std::ifstream file(scriptPath);
        if (!file)
            return {false, 0, "cannot read script " + geometry::occt::toUtf8(scriptPath)};

        std::ostringstream content;
        content << file.rdbuf();
        return run(content.str(), scriptPath.parent_path());
    }

    BatchRunner::Result BatchRunner::run(std::string_view scriptJson, const std::filesystem::path& baseDir)
    {
        Result result;

        json script;
        try
        {
            script = json::parse(scriptJson);
        }
        catch (const json::parse_error& error)
        {
            result.error = std::string("invalid JSON: ") + error.what();
            return result;
        }
        if (!script.is_object() || !script.contains("steps") || !script.at("steps").is_array())
        {
            result.error = "script must be an object with a \"steps\" array";
            return result;
        }

        const json&       steps = script.at("steps");
        const std::size_t total = steps.size();
        for (std::size_t i = 0; i < total; ++i)
        {
            const json&       step   = steps[i];
            const std::string prefix = "step " + std::to_string(i + 1) + "/" + std::to_string(total);

            if (!step.is_object() || step.size() != 1)
            {
                result.error = prefix + ": each step must be an object with exactly one action";
                return result;
            }
            const auto         entry  = step.begin();
            const std::string& action = entry.key();
            const json&        args   = entry.value();

            try
            {
                if (action == "open")
                {
                    const auto path = resolve(args, baseDir);
                    _run(tasks::importTask(_scene, _editor, path));
                    const std::size_t index = _scene.size() - 1;
                    _log << "[batch] " << prefix << ": open " << geometry::occt::toUtf8(path) << " -> model " << index << " '"
                         << _scene.models()[index].name << "' in "
                         << platform::benchmark::formatMilliseconds(_scene.models()[index].loadDuration) << "\n";
                }
                else if (action == "transform")
                {
                    if (!args.is_object())
                        throw StepError("expected an object");
                    const auto                      move   = vector3(args, "move");
                    const auto                      rotate = vector3(args, "rotate");
                    const int                       index  = target(args);
                    const geometry::occt::Transform transform{move[0], move[1], move[2], rotate[0], rotate[1], rotate[2]};
                    const bool                      all = index == service::SceneService::kAllModels;
                    if (_scene.empty() || (!all && (index < 0 || static_cast<std::size_t>(index) >= _scene.size())))
                        throw StepError("no such target (" + std::to_string(_scene.size()) + " model(s) loaded)");
                    for (const auto id : all ? _scene.ids() : std::vector<service::ModelId>{_scene.models()[static_cast<std::size_t>(index)].id})
                        _run(tasks::moveTask(_scene, _editor, id, transform));
                    _log << "[batch] " << prefix << ": transform "
                         << (index == service::SceneService::kAllModels ? std::string("all") : std::to_string(index))
                         << "\n";
                }
                else if (action == "export")
                {
                    const auto path = resolve(args, baseDir);
                    if (_scene.empty())
                        throw StepError("could not export to " + geometry::occt::toUtf8(path) + " (no model loaded)");
                    _run(tasks::exportTask(_scene, path));
                    _log << "[batch] " << prefix << ": export " << geometry::occt::toUtf8(path) << "\n";
                }
                else if (action == "clear")
                {
                    _run(tasks::clearTask(_scene, _editor));
                    _log << "[batch] " << prefix << ": clear\n";
                }
                else
                {
                    throw StepError("unknown action \"" + action + "\"");
                }
            }
            catch (const std::exception& error)
            {
                result.error = prefix + " (" + action + "): " + error.what();
                return result;
            }
            ++result.stepsRun;
        }

        result.ok = true;
        return result;
    }
} // namespace application::workflow
