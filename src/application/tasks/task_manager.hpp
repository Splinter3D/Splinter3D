#pragma once

#include "task.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace application::tasks
{
    /**
     * @brief Runs Tasks in the background, in FIFO order per model.
     *
     * Every public member is called from one thread (the UI thread, or the
     * batch thread); work() runs on the Dispatcher's pool and its results come
     * back through the Poster. A queued task starts when no earlier unfinished
     * task writes a model it reads or writes, and (if it has work) the
     * Dispatcher has a free worker. Reads don't block later writes: prepare()
     * snapshots what work() needs. A task without work whose models are free
     * applies inside submit() and never shows up (an instant edit).
     *
     * Workers freed by any top-level Dispatcher run (its own tasks or not)
     * wake the tasks waiting for workers (Dispatcher::onCapacityFreed()).
     *
     * Without a Poster (synchronous mode, for batch scripts) submit() runs
     * every step inline, in order, and returns once the task is over.
     */
    class TaskManager final
    {
      public:
        /** @brief Runs fn later on the owning thread (e.g. wxEvtHandler::CallAfter). Called from pool threads. */
        using Poster = std::function<void(std::function<void()>)>;

        /** @brief Told about every change to a task's card; removed is true once it is gone. */
        using Listener = std::function<void(const TaskSnapshot&, bool removed)>;

        /** @brief Progress reaches the listener at most this often per task. */
        static constexpr std::chrono::milliseconds kProgressInterval{50};

        explicit TaskManager(Poster poster = {});

        /** @brief cancelAllAndWait(). */
        ~TaskManager();

        TaskManager(const TaskManager&)            = delete;
        TaskManager& operator=(const TaskManager&) = delete;

        /** @brief Queues task; returns 0 (and drops it) after cancelAllAndWait(). */
        TaskId submit(Task task);

        /** @brief Drops a queued task, or asks a running one to stop (its apply() never runs). */
        void cancel(TaskId id);

        /** @brief Removes a Failed task's card. */
        void dismiss(TaskId id);

        /**
         * @brief Cancels every task and blocks until no work runs any more;
         * callbacks still in flight then do nothing. The manager accepts no
         * further work afterwards.
         */
        void cancelAllAndWait();

        /** @brief Tasks queued, running or cancelling (Failed cards don't count). */
        [[nodiscard]] std::size_t pending() const;
        [[nodiscard]] bool        idle() const;

        /**
         * @brief Whether a write to this model submitted now would queue: a
         * pending task writes it, or a queued one will read it (and has no
         * snapshot yet).
         */
        [[nodiscard]] bool wouldWait(service::ModelId id) const;

        /** @brief Whether a queued or running (not cancelling) task of this operation writes `id`. */
        [[nodiscard]] bool hasPending(std::string_view operation, service::ModelId id) const;

        /** @brief A pending or Failed task; empty once it is done, cancelled or dismissed. */
        [[nodiscard]] std::optional<TaskSnapshot> snapshot(TaskId id) const;
        [[nodiscard]] std::vector<TaskSnapshot>   snapshots() const;

        void setListener(Listener listener);

      private:
        struct Entry
        {
            TaskId                       id = 0;
            Task                         task;
            TaskState                    state = TaskState::Queued;
            std::string                  waitingFor; // Queued only
            std::string                  error;      // Failed only
            std::shared_ptr<TaskContext> context;    // from start on
        };

        // Outlives the manager while work is in flight, so late callbacks can
        // tell it is gone.
        struct Shared
        {
            std::mutex               mutex;
            std::condition_variable  settled;
            std::size_t              inFlight = 0;     // submitted works whose callbacks are not posted yet
            bool                     stopped  = false; // UI thread only
            std::atomic<std::size_t> launching{0};
            std::atomic<bool>        rescheduleQueued{false}; // one posted reschedule covers a burst of releases     // submitted works not yet holding their workers
        };

        Entry*                     _find(TaskId id);
        const Entry*               _find(TaskId id) const;
        [[nodiscard]] const Entry* _blocker(const Entry& entry) const;
        [[nodiscard]] TaskSnapshot _snapshot(const Entry& entry) const;

        void        _schedule();
        void        _start(Entry& entry);
        void        _finish(TaskId id, std::optional<std::string> error);
        void        _progressed(TaskId id);
        void        _fail(Entry& entry, std::string error);
        void        _remove(TaskId id);
        void        _notify(const Entry& entry, bool removed = false);
        void        _tell(const TaskSnapshot& snapshot, bool removed) noexcept;
        static void _discard(Entry& entry) noexcept;
        void        _runInline(Entry& entry);
        // Runs a prepare()/apply() step; an exception becomes the task's error.
        [[nodiscard]] static std::optional<std::string> _guarded(const std::function<std::optional<std::string>()>& step);

        Poster                  _poster;
        Listener                _listener;
        std::vector<Entry>      _entries; // submission order
        TaskId                  _nextId        = 1;
        bool                    _scheduling    = false;
        std::shared_ptr<Shared> _shared        = std::make_shared<Shared>();
        std::uint64_t           _capacityToken = 0; // Dispatcher::onCapacityFreed() subscription
    };
} // namespace application::tasks
