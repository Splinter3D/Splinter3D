#pragma once

#include "application/service/scene_service.hpp"

#include <chrono>
#include <cstddef>
#include <deque>
#include <optional>
#include <string>
#include <vector>

namespace application::commands
{
    /**
     * @brief Undo/redo of the edits to a scene, as the states it was in.
     *
     * Before each edit, its owner record()s the scene as it is; undo() hands
     * that state back (keeping the current one for redo()), and the owner
     * puts the scene back in it. Shapes are handles: a state costs a copy of
     * the model list, not of the meshes, but keeps a removed model's mesh
     * alive while it can still be brought back.
     *
     * UI thread only.
     */
    class History final
    {
      public:
        using Clock = std::chrono::steady_clock;

        /** @brief The models (B-Reps left out: a cache, rebuilt on demand), and whether each is shown. */
        struct State
        {
            std::vector<service::SceneService::Model> models;
            std::vector<bool>                         visibility; // same order as models
        };

        /// The most steps kept; the oldest go first.
        static constexpr std::size_t kLimit = 100;

        /// Edits with the same merge key less than this apart are one step.
        static constexpr std::chrono::milliseconds kMergeWindow{1000};

        /**
         * @brief Records the state before an edit as a new undo step, and
         * forgets what could be redone.
         *
         * @param mergeKey Non-empty: when the last step had the same key and
         * was recorded less than kMergeWindow ago (and nothing was undone
         * since), this edit joins it (e.g. a burst of changes to one model's
         * fields), and `before` is dropped.
         */
        void record(State before, std::string mergeKey = {}, Clock::time_point now = Clock::now());

        [[nodiscard]] bool canUndo() const noexcept;
        [[nodiscard]] bool canRedo() const noexcept;

        /** @brief The state to go back to (current is kept for redo()); empty if there is none. */
        [[nodiscard]] std::optional<State> undo(State current);

        /** @brief The state to go forward to (current is kept for undo()); empty if there is none. */
        [[nodiscard]] std::optional<State> redo(State current);

        void clear() noexcept;

      private:
        std::deque<State>  undo_;
        std::vector<State> redo_;
        std::string        mergeKey_;
        Clock::time_point  lastRecord_{};
    };
} // namespace application::commands
