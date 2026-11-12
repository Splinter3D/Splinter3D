#include "scene_service.hpp"

#include "geometry/occt/path.hpp"
#include "platform/logger/logger.hpp"
#include "platform/threading/dispatcher.hpp"

#include <BRepBuilderAPI_Copy.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Builder.hxx>
#include <IMeshTools_Parameters.hxx>
#include <Message_ProgressScope.hxx>
#include <StlAPI_Writer.hxx>
#include <TopLoc_Location.hxx>
#include <TopoDS_Compound.hxx>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <fstream>
#include <optional>
#include <random>
#include <string>
#include <thread>
#include <utility>

namespace application::service
{
    std::size_t SceneService::load(const std::filesystem::path& path)
    {
        std::optional<platform::benchmark::Report> report;

        auto              shape     = platform::benchmark::measure(&report, platform::benchmark::ALL,
                                                                   [&] { return geometry::occt::Shape::fromSTL(path); });
        const std::size_t index     = add(geometry::occt::toUtf8(path.filename()), std::move(shape));
        models_[index].loadDuration = report->time.value_or(platform::benchmark::Duration::zero());
        platform::logger::clog("[scene] imported '", models_[index].name, "' from ", geometry::occt::toUtf8(path),
                               " (", *report, ")");
        return index;
    }

    std::size_t SceneService::add(const std::string&            baseName,
                                  geometry::occt::Shape         shape,
                                  ModelId                       id,
                                  platform::benchmark::Duration loadDuration)
    {
        Model model;
        model.id           = id != 0 ? id : nextId_++;
        model.name         = uniqueName(baseName);
        model.shape        = std::move(shape);
        model.loadDuration = loadDuration;
        std::erase(reserved_, model.id);
        reservedNames_.erase(model.id);
        models_.push_back(std::move(model));
        return models_.size() - 1;
    }

    ModelId SceneService::reserveId(std::string pendingName)
    {
        reserved_.push_back(nextId_);
        if (!pendingName.empty())
            reservedNames_.emplace(nextId_, std::move(pendingName));
        return nextId_++;
    }

    std::string SceneService::reservedName(ModelId id) const
    {
        const auto found = reservedNames_.find(id);
        return found != reservedNames_.end() ? found->second : std::string();
    }

    bool SceneService::setBRep(ModelId id, geometry::occt::Shape brep)
    {
        const auto index = indexOf(id);
        if (!index)
            return false;
        if (auto& old = models_[*index].brep)
            releaseInBackground(std::move(*old));
        models_[*index].brep = std::move(brep);
        return true;
    }

    const geometry::occt::Shape* SceneService::brep(ModelId id) const noexcept
    {
        const Model* model = find(id);
        return model != nullptr && model->brep ? &*model->brep : nullptr;
    }

    void SceneService::releaseId(ModelId id)
    {
        std::erase(reserved_, id);
        reservedNames_.erase(id);
    }

    const std::vector<ModelId>& SceneService::reservedIds() const noexcept
    {
        return reserved_;
    }

    bool SceneService::removeById(ModelId id)
    {
        const auto index = indexOf(id);
        if (!index)
            return false;
        if (auto& brep = models_[*index].brep)
            releaseInBackground(std::move(*brep));
        models_.erase(models_.begin() + static_cast<std::ptrdiff_t>(*index));
        return true;
    }

    const SceneService::Model* SceneService::find(ModelId id) const noexcept
    {
        const auto index = indexOf(id);
        return index ? &models_[*index] : nullptr;
    }

    std::optional<std::size_t> SceneService::indexOf(ModelId id) const noexcept
    {
        for (std::size_t i = 0; i < models_.size(); ++i)
            if (models_[i].id == id)
                return i;
        return std::nullopt;
    }

    std::vector<ModelId> SceneService::ids() const
    {
        std::vector<ModelId> result;
        result.reserve(models_.size());
        for (const auto& model : models_)
            result.push_back(model.id);
        return result;
    }

    bool SceneService::setTransformById(ModelId id, const geometry::occt::Transform& transform)
    {
        const auto index = indexOf(id);
        if (!index)
            return false;
        models_[*index].transform = transform;
        return true;
    }

    void SceneService::clear() noexcept
    {
        for (auto& model : models_)
            if (model.brep)
                releaseInBackground(std::move(*model.brep));
        models_.clear();
    }

    bool SceneService::remove(int index)
    {
        if (!isValidIndex(index))
            return false;
        if (auto& brep = models_[static_cast<std::size_t>(index)].brep)
            releaseInBackground(std::move(*brep));
        models_.erase(models_.begin() + index);
        return true;
    }

    bool SceneService::empty() const noexcept
    {
        return models_.empty();
    }

    std::size_t SceneService::size() const noexcept
    {
        return models_.size();
    }

    const std::vector<SceneService::Model>& SceneService::models() const noexcept
    {
        return models_;
    }

    std::vector<std::string> SceneService::names() const
    {
        std::vector<std::string> names;
        names.reserve(models_.size());
        for (const auto& model : models_)
            names.push_back(model.name);
        return names;
    }

    geometry::occt::Transform SceneService::transform(int targetIndex) const
    {
        if (!isValidIndex(targetIndex))
            return {};
        return models_[static_cast<std::size_t>(targetIndex)].transform;
    }

    bool SceneService::setTransform(int targetIndex, const geometry::occt::Transform& transform)
    {
        if (targetIndex == kAllModels && !models_.empty())
        {
            for (auto& model : models_)
                model.transform = transform;
            return true;
        }
        if (!isValidIndex(targetIndex))
            return false;
        models_[static_cast<std::size_t>(targetIndex)].transform = transform;
        return true;
    }

    bool SceneService::saveSTL(const std::filesystem::path& path) const
    {
        return exportSTL(models_, path);
    }

    bool SceneService::isValidIndex(int index) const noexcept
    {
        return index >= 0 && static_cast<std::size_t>(index) < models_.size();
    }

    // Disambiguates two models loaded from files with the same name (e.g.
    // imported from different folders) so the target dropdown never shows
    // duplicate entries.
    std::string SceneService::uniqueName(const std::string& base) const
    {
        const auto taken = [this](const std::string& name) {
            return std::any_of(models_.begin(), models_.end(),
                               [&](const Model& model) { return model.name == name; });
        };

        std::string candidate = base;
        for (int suffix = 2; taken(candidate); ++suffix)
            candidate = base + " (" + std::to_string(suffix) + ")";
        return candidate;
    }

    std::optional<std::filesystem::path> writeSTLTemporary(const std::vector<SceneService::Model>& models,
                                                           const std::filesystem::path&            path,
                                                           const Message_ProgressRange&            progress)
    {
        if (models.empty())
            return std::nullopt;

        // Every model, each placed where the viewer shows it, in one compound.
        // Moved() only attaches a location to the shared geometry; the writer
        // applies it to the triangles it outputs. Mesh models are written as
        // they are; only shapes without a mesh (e.g. results of B-Rep
        // operations) need meshing first.
        BRep_Builder    builder;
        TopoDS_Compound exported;
        TopoDS_Compound brepParts;
        builder.MakeCompound(exported);
        builder.MakeCompound(brepParts);
        bool hasBRep = false;
        for (const auto& model : models)
        {
            const TopoDS_Shape placed =
                model.shape.value().Moved(TopLoc_Location(geometry::occt::toTrsf(model.transform)));
            if (!model.shape.mesh().IsNull())
                builder.Add(exported, placed);
            else
            {
                builder.Add(brepParts, placed);
                hasBRep = true;
            }
        }

        Message_ProgressScope scope(progress, "Export STL", 2);
        const auto            meshing = scope.Next();
        if (hasBRep)
        {
            // Meshing replaces the faces' triangulations, and the shapes are
            // shared with the scene and the viewer (and other exports, on
            // other threads): work on a deep copy, meshes included.
            BRepBuilderAPI_Copy copier(brepParts, /*copyGeom=*/true, /*copyMesh=*/true);
            const TopoDS_Shape  copy = copier.Shape();

            // StlAPI_Writer only exports faces that already carry a
            // triangulation: mesh them (a no-op where fine enough). 0.01 is
            // relative to each edge's size.
            IMeshTools_Parameters parameters;
            parameters.Deflection = 0.01;
            parameters.Relative   = true;
            parameters.Angle      = 0.5;
            parameters.InParallel = false;
            BRepMesh_IncrementalMesh mesher(copy, parameters, meshing);
            if (scope.UserBreak())
                return std::nullopt;
            builder.Add(exported, copy);
        }

        // Unique, so two exports to the same path never write the same file.
        static std::atomic<std::uint64_t> counter{0};
        std::filesystem::path             temporary = path;
        temporary += "." + std::to_string(std::random_device{}()) + "-" + std::to_string(counter++) + ".tmp";
        bool written = false;
        {
            StlAPI_Writer writer;
            std::ofstream stream(temporary, writer.ASCIIMode() ? std::ios::out : std::ios::out | std::ios::binary);
            written = stream && writer.Write(exported, stream, scope.Next()) && stream.flush().good();
        }

        if (!written || scope.UserBreak())
        {
            std::error_code ec;
            std::filesystem::remove(temporary, ec);
            return std::nullopt;
        }
        return temporary;
    }

    void releaseInBackground(geometry::occt::Shape shape)
    {
        if (shape.isNull())
            return;
        try
        {
            auto holder = std::make_shared<geometry::occt::Shape>(std::move(shape));
            // post(), not submit(): freeing memory takes no CPU budget, and
            // must not make tasks wait for workers.
            platform::threading::Dispatcher::getInstance().post([holder]() mutable { holder.reset(); });
        }
        catch (...)
        {
            // Out of memory: it is freed here, on this thread, instead.
        }
    }

    bool commitSTLTemporary(const std::filesystem::path& temporary, const std::filesystem::path& path)
    {
        // POSIX replaces atomically. Windows (MoveFileExW) refuses with a
        // sharing violation while another export is replacing the same file,
        // or something holds it open for a moment: retry for up to ~1 s.
        constexpr int                       kAttempts = 100;
        constexpr std::chrono::milliseconds kBackoff{10};
        std::error_code                     ec;
        for (int attempt = 0; attempt < kAttempts; ++attempt)
        {
            std::filesystem::rename(temporary, path, ec);
            if (!ec)
                return true;
            std::this_thread::sleep_for(kBackoff);
        }
        std::filesystem::remove(temporary, ec);
        return false;
    }

    bool exportSTL(const std::vector<SceneService::Model>& models,
                   const std::filesystem::path&            path,
                   const Message_ProgressRange&            progress)
    {
        const auto temporary = writeSTLTemporary(models, path, progress);
        return temporary && commitSTLTemporary(*temporary, path);
    }
} // namespace application::service
