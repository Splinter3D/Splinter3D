#pragma once

#include <Message_ProgressIndicator.hxx>
#include <Message_ProgressRange.hxx>
#include <atomic>
#include <functional>
#include <mutex>
#include <optional>
#include <string>

namespace application::tasks
{
    /**
     * @brief What a running task reports, and how it learns it was cancelled.
     *
     * Shared between the task's work (a pool thread) and the TaskManager (UI
     * thread); every member is thread-safe. Work either reports progress
     * itself, or hands progressRange() to an OCCT algorithm, which then
     * reports through it and stops once the task is cancelled.
     */
    class TaskContext final
    {
      public:
        TaskContext();
        ~TaskContext();

        TaskContext(const TaskContext&)            = delete;
        TaskContext& operator=(const TaskContext&) = delete;

        /**
         * @brief Called (on the reporting thread, outside any lock) after each
         * progress or status update. Set it before the work starts.
         */
        void setOnChange(std::function<void()> callback);

        /** @brief Sets the completed fraction, clamped to [0, 1]. */
        void setProgress(double fraction) noexcept;

        /** @brief Drops the fraction (the bar animates instead) and shows status. */
        void setIndeterminate(std::string status);

        [[nodiscard]] bool cancelled() const noexcept;
        void               cancel() noexcept;

        /**
         * @brief A range for OCCT algorithms: their progress becomes this
         * context's, and cancel() makes them stop at their next check.
         * Available once per context.
         *
         * @throws std::logic_error on a second call.
         */
        [[nodiscard]] Message_ProgressRange progressRange();

        /** @brief Completed fraction; empty while indeterminate or not yet reported. */
        [[nodiscard]] std::optional<double> progress() const;

        [[nodiscard]] std::string status() const;

      private:
        class Indicator;

        void _changed() noexcept;

        mutable std::mutex                     _mutex;
        std::optional<double>                  _progress;
        std::string                            _status;
        std::atomic<bool>                      _cancelled{false};
        std::function<void()>                  _onChange;
        occ::handle<Message_ProgressIndicator> _indicator;
    };
} // namespace application::tasks
