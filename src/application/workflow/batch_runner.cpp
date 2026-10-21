#include "batch_runner.hpp"

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
    { }

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
            const std::string& action = step.begin().key();
            const json&        args   = step.begin().value();

            try
            {
                if (action == "open")
                {
                    const auto        path  = resolve(args, baseDir);
                    const std::size_t index = _scene.load(path);
                    _log << "[batch] " << prefix << ": open " << geometry::occt::toUtf8(path) << " -> model " << index << " '"
                         << _scene.models()[index].name << "'\n";
                }
                else if (action == "transform")
                {
                    if (!args.is_object())
                        throw StepError("expected an object");
                    const auto move   = vector3(args, "move");
                    const auto rotate = vector3(args, "rotate");
                    const int  index  = target(args);
                    if (!_scene.setTransform(index, {move[0], move[1], move[2], rotate[0], rotate[1], rotate[2]}))
                        throw StepError("no such target (" + std::to_string(_scene.size()) + " model(s) loaded)");
                    _log << "[batch] " << prefix << ": transform "
                         << (index == service::SceneService::kAllModels ? std::string("all") : std::to_string(index))
                         << "\n";
                }
                else if (action == "export")
                {
                    const auto path = resolve(args, baseDir);
                    if (!_scene.saveSTL(path))
                        throw StepError("could not export to " + geometry::occt::toUtf8(path) +
                                        (_scene.empty() ? " (no model loaded)" : ""));
                    _log << "[batch] " << prefix << ": export " << geometry::occt::toUtf8(path) << "\n";
                }
                else if (action == "clear")
                {
                    _scene.clear();
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
