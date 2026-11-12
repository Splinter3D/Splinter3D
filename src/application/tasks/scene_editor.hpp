#pragma once

#include "application/service/scene_service.hpp"

#include <string>

namespace application::tasks
{
    /**
     * @brief Commits a change to the scene everywhere it must show.
     *
     * Tasks change the scene only through this, from their apply() step on
     * the UI thread. The GUI implements it to keep the viewer's presentations
     * in step with the SceneService; batch mode uses SceneServiceEditor.
     */
    class SceneEditor
    {
      public:
        virtual ~SceneEditor() = default;

        virtual void addModel(service::ModelId              id,
                              const std::string&            baseName,
                              geometry::occt::Shape         shape,
                              platform::benchmark::Duration loadDuration)                          = 0;
        virtual void removeModel(service::ModelId id)                                              = 0;
        virtual void setTransform(service::ModelId id, const geometry::occt::Transform& transform) = 0;
        virtual void clear()                                                                       = 0;
        // Caches a model's B-Rep; what is displayed does not change.
        virtual void setBRep(service::ModelId id, geometry::occt::Shape brep) = 0;
        // Replaces a model's shape with the same geometry stored differently (see SceneService::setShape).
        virtual void setShape(service::ModelId id, geometry::occt::Shape shape) = 0;

      protected:
        SceneEditor()                              = default;
        SceneEditor(const SceneEditor&)            = default;
        SceneEditor& operator=(const SceneEditor&) = default;
    };

    /** @brief Applies changes to a SceneService only (no viewer): batch mode and tests. */
    class SceneServiceEditor final : public SceneEditor
    {
      public:
        explicit SceneServiceEditor(service::SceneService& scene);

        void addModel(service::ModelId              id,
                      const std::string&            baseName,
                      geometry::occt::Shape         shape,
                      platform::benchmark::Duration loadDuration) override;
        void removeModel(service::ModelId id) override;
        void setTransform(service::ModelId id, const geometry::occt::Transform& transform) override;
        void clear() override;
        void setBRep(service::ModelId id, geometry::occt::Shape brep) override;
        void setShape(service::ModelId id, geometry::occt::Shape shape) override;

      private:
        service::SceneService& _scene;
    };
} // namespace application::tasks
