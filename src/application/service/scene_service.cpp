#include "scene_service.hpp"

#include "geometry/occt/path.hpp"

#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Builder.hxx>
#include <StlAPI_Writer.hxx>
#include <TopLoc_Location.hxx>
#include <TopoDS_Compound.hxx>
#include <algorithm>
#include <utility>

namespace application::service
{
    std::size_t SceneService::load(const std::filesystem::path& path)
    {
        return add(geometry::occt::toUtf8(path.filename()), geometry::occt::Shape::fromSTL(path));
    }

    std::size_t SceneService::add(const std::string& baseName, geometry::occt::Shape shape)
    {
        models_.push_back(Model{uniqueName(baseName), std::move(shape), {}});
        return models_.size() - 1;
    }

    void SceneService::clear() noexcept
    {
        models_.clear();
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
        if (models_.empty())
            return false;

        // Every model, each placed where the viewer shows it, in one compound.
        // Moved() only attaches a location to the shared geometry; the writer
        // applies it to the triangles it outputs.
        TopoDS_Compound compound;
        BRep_Builder    builder;
        builder.MakeCompound(compound);
        for (const auto& model : models_)
            builder.Add(compound,
                        model.shape.value().Moved(TopLoc_Location(geometry::occt::toTrsf(model.transform))));

        // StlAPI_Writer only exports faces that already carry a triangulation.
        // The viewer happens to create one when it displays a shape, but export
        // must not depend on that: mesh here (a no-op if already meshed finely
        // enough). 0.01 is relative to each edge's size.
        BRepMesh_IncrementalMesh mesher(compound, 0.01, /*isRelative=*/true, 0.5, /*isInParallel=*/false);
        (void) mesher;

        StlAPI_Writer writer;
        return writer.Write(compound, geometry::occt::toUtf8(path).c_str());
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
} // namespace application::service
