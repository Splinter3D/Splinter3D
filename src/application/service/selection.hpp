#pragma once

#include "scene_service.hpp"

#include <cstddef>
#include <functional>
#include <vector>

namespace application::service
{
    /**
     * @brief The models the user picked, by id (so removing an earlier model
     * doesn't shift them), in the order they were picked.
     *
     * Holds ids only: whether they still exist is the SceneService's business
     * (see retainOnly()). UI thread only.
     */
    class Selection
    {
      public:
        /// The selection becomes just this model.
        void selectOnly(ModelId id);
        /// The selection becomes these models, in this order (repeats dropped).
        void set(const std::vector<ModelId>& ids);
        /// Adds the model, or removes it if it was selected.
        void toggle(ModelId id);
        void clear();
        /// Removes one model; true if it was selected.
        bool remove(ModelId id);
        /// Drops every selected model not in existing.
        void retainOnly(const std::vector<ModelId>& existing);

        [[nodiscard]] bool                        contains(ModelId id) const noexcept;
        [[nodiscard]] const std::vector<ModelId>& ids() const noexcept;
        [[nodiscard]] std::size_t                 size() const noexcept;
        [[nodiscard]] bool                        empty() const noexcept;

        /// Called once after each call that changed the selection.
        void setOnChanged(std::function<void()> callback);

      private:
        void notify() const;

        std::vector<ModelId>  ids_;
        std::function<void()> onChanged_;
    };
} // namespace application::service
