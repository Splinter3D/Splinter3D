#pragma once

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace platform::threading
{
    /**
     * @brief A fixed set of worker threads sharing one FIFO task queue.
     *
     * parallelFor() is the only way in. It splits the range into slices that
     * the calling thread and the queued tasks claim one at a time; the caller
     * claims slices until none is left, then only waits for slices other
     * threads are already running. So a parallelFor() nested in another one
     * cannot deadlock on a full pool, and a caller never runs another call's
     * work. Each slice runs under the submitter's ExecutionContext with its
     * share of maxWorkers as the budget, so nested work stays within it.
     */
    class WorkerPool final
    {
      public:
        /** @brief Starts max(threads, 1) worker threads. */
        explicit WorkerPool(std::size_t threads);

        /** @brief Runs what is still queued, then joins every worker. */
        ~WorkerPool();

        WorkerPool(const WorkerPool&)            = delete;
        WorkerPool& operator=(const WorkerPool&) = delete;

        /** @brief Number of worker threads (the calling thread not included). */
        [[nodiscard]] std::size_t size() const noexcept;

        /**
         * @brief Calls body(i) once for each i in [first, last), on at most
         * maxWorkers threads including the caller, and returns when all calls
         * are done. If calls throw, the first exception is rethrown here once
         * every other call has finished. An empty or reversed range does
         * nothing.
         */
        void parallelFor(std::size_t first, std::size_t last, std::size_t maxWorkers,
                         const std::function<void(std::size_t)>& body);

        /**
         * @brief Runs task once on a pool thread, outside any
         * ExecutionContext, and returns at once. task must not throw.
         */
        void post(std::function<void()> task);

      private:
        using Task = std::function<void()>;

        void _workerLoop();

        std::mutex               _mutex;
        std::condition_variable  _wake;
        std::deque<Task>         _queue;
        bool                     _stopping = false;
        std::vector<std::thread> _threads;
    };
} // namespace platform::threading
