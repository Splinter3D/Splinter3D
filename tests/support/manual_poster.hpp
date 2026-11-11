#pragma once

#include "application/tasks/task_manager.hpp"

#include <chrono>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace tests::support
{
    // Stands in for the UI thread: callbacks posted by workers wait here until
    // the test drains them (on its own thread, like CallAfter on the UI thread).
    class ManualPoster
    {
      public:
        application::tasks::TaskManager::Poster poster()
        {
            return [this](std::function<void()> fn) {
                const std::lock_guard lock(_mutex);
                _queue.push_back(std::move(fn));
            };
        }

        [[nodiscard]] std::size_t size()
        {
            const std::lock_guard lock(_mutex);
            return _queue.size();
        }

        // Runs posted callbacks until condition() holds; false on timeout.
        bool drainUntil(const std::function<bool()>& condition, std::chrono::milliseconds timeout = std::chrono::seconds(5))
        {
            const auto deadline = std::chrono::steady_clock::now() + timeout;
            while (!condition())
            {
                if (std::chrono::steady_clock::now() > deadline)
                    return false;
                std::vector<std::function<void()>> batch;
                {
                    const std::lock_guard lock(_mutex);
                    batch.swap(_queue);
                }
                for (auto& fn : batch)
                    fn();
                if (batch.empty())
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            return true;
        }

      private:
        std::mutex                         _mutex;
        std::vector<std::function<void()>> _queue;
    };

} // namespace tests::support
