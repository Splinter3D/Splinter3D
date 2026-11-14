#pragma once

#include "application/service/scene_service.hpp"
#include "task_context.hpp"

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace application::tasks
{
    /** @brief Identifies a submitted task; never reused within a TaskManager. */
    using TaskId = std::uint64_t;

    /** @brief Where a task stands. Done and cancelled tasks are simply gone. */
    enum class TaskState
    {
        Queued,
        Running,
        Cancelling,
        Failed,
    };

    /** @brief A task as the UI shows it. */
    struct TaskSnapshot
    {
        TaskId                id = 0;
        std::string           name;
        TaskState             state = TaskState::Queued;
        std::optional<double> progress; // empty: indeterminate
        std::string           status;   // e.g. "Writing…", "Queued · waiting for workers"
        std::string           error;    // Failed only
    };

    /**
     * @brief An operation on the scene, run by a TaskManager in three steps:
     * prepare() and apply() on the UI thread, work() in the background.
     *
     * The models it reads and writes order it against other tasks (FIFO per
     * model, see TaskManager). work() must only use what prepare() captured,
     * never the live scene.
     */
    struct Task
    {
        std::string name;      // shown on its card, e.g. "Import cube.stl"
        std::string operation; // Dispatcher profile key; empty when there is no work()

        std::vector<service::ModelId> reads;
        std::vector<service::ModelId> writes;

        /** @brief UI thread, when the task starts: checks and snapshots its models. An error fails the task. */
        std::function<std::optional<std::string>()> prepare;

        /** @brief Background; throws to fail. Empty for instant edits. */
        std::function<void(TaskContext&)> work;

        /** @brief UI thread, after work() succeeded: commits the result. */
        std::function<void()> apply;

        /**
         * @brief UI thread, when the task ends without apply() (cancelled,
         * failed, or dropped at shutdown): undoes what prepare() or work()
         * left behind, e.g. a temporary file. Must not throw.
         */
        std::function<void()> discard;
    };
} // namespace application::tasks
