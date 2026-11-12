#include "model_tasks.hpp"

#include "geometry/occt/path.hpp"
#include "platform/benchmark/benchmark.hpp"
#include "platform/logger/logger.hpp"
#include "platform/threading/dispatcher.hpp"

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace application::tasks
{
    namespace
    {
        constexpr const char* kImportOperation      = "model.import";
        constexpr const char* kLargeImportOperation = "model.import.large";
        constexpr const char* kExportOperation      = "model.export";
        constexpr const char* kConvertOperation     = "model.convert";

        // Every model, plus those earlier imports will add: a task on "the
        // whole scene" must come after them.
        std::vector<service::ModelId> wholeScene(const service::SceneService& scene)
        {
            std::vector<service::ModelId> ids      = scene.ids();
            const auto&                   reserved = scene.reservedIds();
            ids.insert(ids.end(), reserved.begin(), reserved.end());
            return ids;
        }

        std::string nameOf(const service::SceneService& scene, service::ModelId id)
        {
            if (const auto* model = scene.find(id))
                return model->name;
            const std::string pending = scene.reservedName(id); // still being imported
            return pending.empty() ? "model" : pending;
        }

        // Fails the task if the model went away before its turn came.
        std::function<std::optional<std::string>()> requireModel(const service::SceneService& scene,
                                                                 service::ModelId             id,
                                                                 std::string                  name)
        {
            return [&scene, id, name = std::move(name)]() -> std::optional<std::string> {
                if (scene.find(id) == nullptr)
                    return "model \"" + name + "\" no longer exists";
                return std::nullopt;
            };
        }
    } // namespace

    std::string importOperation(std::uintmax_t bytes)
    {
        return bytes >= kLargeImportBytes ? kLargeImportOperation : kImportOperation;
    }

    Task importTask(service::SceneService& scene, SceneEditor& editor, std::filesystem::path path)
    {
        struct Result
        {
            geometry::occt::Shape       shape;
            platform::benchmark::Report report{};
        };

        const std::string      baseName = geometry::occt::toUtf8(path.filename());
        const service::ModelId id       = scene.reserveId(baseName);
        auto                   result   = std::make_shared<Result>();

        Task task;
        task.name = "Import " + baseName;
        std::error_code ec;
        const auto      bytes = std::filesystem::file_size(path, ec);
        task.operation        = importOperation(ec ? 0 : bytes);
        task.writes           = {id};
        task.work             = [path, result](TaskContext& context) {
            std::optional<platform::benchmark::Report> report;
            result->shape  = platform::benchmark::measure(&report, platform::benchmark::Metrics::NOFPS,
                                                          [&] {
                                                             return geometry::occt::Shape::fromSTL(
                                                                 path, context.progressRange(),
                                                                 [](std::size_t first, std::size_t last, const std::function<void(std::size_t)>& body) {
                                                                     platform::threading::Dispatcher::getInstance().parallelFor(first, last, body);
                                                                 });
                                                          });
            result->report = report.value_or(platform::benchmark::Report{});
        };
        task.discard = [&scene, id] { scene.releaseId(id); };
        task.apply   = [&editor, id, baseName, path, result] {
            platform::logger::clog("[scene] imported '", baseName, "' from ", geometry::occt::toUtf8(path), " (",
                                   result->report, ")");
            editor.addModel(id, baseName, std::move(result->shape), result->report.time.value_or(platform::benchmark::Duration::zero()));
        };
        return task;
    }

    Task exportTask(const service::SceneService& scene, std::filesystem::path path)
    {
        auto snapshot = std::make_shared<std::vector<service::SceneService::Model>>();

        Task task;
        task.name      = "Export " + geometry::occt::toUtf8(path.filename());
        task.operation = kExportOperation;
        task.reads     = wholeScene(scene);
        // Only the models asked for: one added by a later task is not part of this export.
        task.prepare = [&scene, reads = task.reads, snapshot]() -> std::optional<std::string> {
            snapshot->clear();
            for (const auto& model : scene.models())
                if (std::find(reads.begin(), reads.end(), model.id) != reads.end())
                {
                    snapshot->push_back(model);
                    snapshot->back().brep.reset(); // not needed, and must not outlive the model
                }
            if (snapshot->empty())
                return "no model loaded";
            return std::nullopt;
        };
        // The file is written beside path in the background, and only
        // replaces it in apply(): a cancel, even after the writing, leaves
        // path as it was.
        auto temporary = std::make_shared<std::optional<std::filesystem::path>>();
        task.work      = [path, snapshot, temporary](TaskContext& context) {
            *temporary = service::writeSTLTemporary(*snapshot, path, context.progressRange());
            if (*temporary)
                return;
            if (context.cancelled())
                throw std::runtime_error("export cancelled");
            throw std::runtime_error("could not export to " + geometry::occt::toUtf8(path));
        };
        task.apply = [path, temporary] {
            const auto written = std::exchange(*temporary, std::nullopt);
            if (!written || !service::commitSTLTemporary(*written, path))
                throw std::runtime_error("could not export to " + geometry::occt::toUtf8(path));
        };
        task.discard = [temporary] {
            if (*temporary)
            {
                std::error_code ec;
                std::filesystem::remove(**temporary, ec);
            }
        };
        return task;
    }

    Task moveTask(const service::SceneService& scene,
                  SceneEditor&                 editor,
                  service::ModelId             id,
                  geometry::occt::Transform    transform)
    {
        const std::string name = nameOf(scene, id);

        Task task;
        task.name    = "Move " + name;
        task.writes  = {id};
        task.prepare = requireModel(scene, id, name);
        task.apply   = [&editor, id, transform] { editor.setTransform(id, transform); };
        return task;
    }

    Task removeTask(const service::SceneService& scene, SceneEditor& editor, service::ModelId id)
    {
        const std::string name = nameOf(scene, id);

        Task task;
        task.name    = "Remove " + name;
        task.writes  = {id};
        task.prepare = requireModel(scene, id, name);
        task.apply   = [&editor, id] { editor.removeModel(id); };
        return task;
    }

    Task clearTask(const service::SceneService& scene, SceneEditor& editor)
    {
        Task task;
        task.name   = "Clear scene";
        task.writes = wholeScene(scene);
        task.apply  = [&editor] { editor.clear(); };
        return task;
    }

    Task convertTask(const service::SceneService& scene, SceneEditor& editor, service::ModelId id)
    {
        const std::string name   = nameOf(scene, id);
        auto              mesh   = std::make_shared<geometry::occt::Shape>();
        auto              result = std::make_shared<geometry::occt::Shape>();

        Task task;
        task.name      = "Convert " + name;
        task.operation = kConvertOperation;
        task.reads     = {id};
        task.writes    = {brepResource(id)};
        task.prepare   = [&scene, id, name, mesh]() -> std::optional<std::string> {
            const auto* model = scene.find(id);
            if (model == nullptr)
                return "model \"" + name + "\" no longer exists";
            *mesh = model->shape; // a handle copy: the triangulation is shared, not copied
            return std::nullopt;
        };
        task.work = [mesh, result](TaskContext& context) {
            *result = mesh->mesh().IsNull() ? *mesh // already a B-Rep (e.g. a cut's result)
                                            : geometry::occt::Shape::toBRep(*mesh, context.progressRange());
        };
        task.apply = [&scene, &editor, id, result] {
            if (scene.find(id) != nullptr)
                editor.setBRep(id, std::move(*result));
            else // removed while converting
                service::releaseInBackground(std::move(*result));
        };
        task.discard = [result] { service::releaseInBackground(std::move(*result)); };
        return task;
    }

    std::function<std::optional<std::string>()> requireBRep(const service::SceneService& scene, service::ModelId id)
    {
        return [&scene, id, name = nameOf(scene, id)]() -> std::optional<std::string> {
            if (scene.find(id) == nullptr)
                return "model \"" + name + "\" no longer exists";
            if (scene.brep(id) == nullptr)
                return "model \"" + name + "\" could not be converted";
            return std::nullopt;
        };
    }

    std::vector<TaskId> submitWithBRep(TaskManager&                  tasks,
                                       const service::SceneService&  scene,
                                       SceneEditor&                  editor,
                                       std::vector<service::ModelId> models,
                                       Task                          operation)
    {
        std::vector<TaskId> submitted;
        for (const auto id : models)
        {
            if (scene.brep(id) == nullptr && !tasks.hasPending(kConvertOperation, brepResource(id)))
                submitted.push_back(tasks.submit(convertTask(scene, editor, id)));
            operation.reads.push_back(brepResource(id)); // after the conversion, whichever it is
        }
        submitted.push_back(tasks.submit(std::move(operation)));
        return submitted;
    }
} // namespace application::tasks
