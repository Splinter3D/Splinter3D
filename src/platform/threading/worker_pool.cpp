#include "worker_pool.hpp"

#include "execution_context.hpp"

#include <algorithm>
#include <atomic>
#include <exception>
#include <memory>
#include <optional>
#include <utility>

namespace platform::threading
{
    namespace
    {
        // Shared by one parallelFor() call and its queued slices; outlives the
        // call if a slice is still signalling when the caller returns.
        struct Batch
        {
            std::atomic<std::size_t> next{0};
            std::atomic<std::size_t> remaining{0};
            std::mutex               mutex;
            std::condition_variable  done;
            std::exception_ptr       error;
        };
    } // namespace

    WorkerPool::WorkerPool(std::size_t threads)
    {
        const std::size_t count = std::max<std::size_t>(threads, 1);
        _threads.reserve(count);
        for (std::size_t i = 0; i < count; ++i)
            _threads.emplace_back(&WorkerPool::_workerLoop, this);
    }

    WorkerPool::~WorkerPool()
    {
        {
            const std::lock_guard lock(_mutex);
            _stopping = true;
        }
        _wake.notify_all();
        for (auto& thread : _threads)
            thread.join();
    }

    std::size_t WorkerPool::size() const noexcept
    {
        return _threads.size();
    }

    void WorkerPool::parallelFor(std::size_t first, std::size_t last, std::size_t maxWorkers,
                                 const std::function<void(std::size_t)>& body)
    {
        if (first >= last)
            return;

        const std::size_t count  = last - first;
        const std::size_t slices = std::min(std::max<std::size_t>(maxWorkers, 1), count);
        const std::size_t base   = count / slices;
        const std::size_t extra  = count % slices;

        auto batch       = std::make_shared<Batch>();
        batch->remaining = slices;

        const std::size_t budget  = std::max<std::size_t>(maxWorkers, 1);
        const std::size_t share   = budget / slices;
        const std::size_t spare   = budget % slices;
        const auto        context = ExecutionContext::current();

        auto runSlice = [&body, batch, context, first, base, extra, share, spare](std::size_t k) {
            const std::size_t begin = first + k * base + std::min(k, extra);
            const std::size_t end   = begin + base + (k < extra ? 1 : 0);
            try
            {
                std::optional<ExecutionContext> sliceContext = context;
                if (sliceContext)
                    sliceContext->budget = share + (k < spare ? 1 : 0);
                const ExecutionContext::Scope scope(std::move(sliceContext));
                for (std::size_t i = begin; i < end; ++i)
                    body(i);
            }
            catch (...)
            {
                const std::lock_guard lock(batch->mutex);
                if (!batch->error)
                    batch->error = std::current_exception();
            }
            if (--batch->remaining == 0)
            {
                const std::lock_guard lock(batch->mutex);
                batch->done.notify_all();
            }
        };

        auto claimSlices = [runSlice, batch, slices] {
            for (std::size_t k = batch->next++; k < slices; k = batch->next++)
                runSlice(k);
        };

        try
        {
            const std::lock_guard lock(_mutex);
            for (std::size_t k = 1; k < slices; ++k)
                _queue.emplace_back(claimSlices);
        }
        catch (...)
        {
        }
        if (slices > 1)
            _wake.notify_all();

        claimSlices();

        {
            std::unique_lock lock(batch->mutex);
            batch->done.wait(lock, [&] { return batch->remaining == 0; });
        }

        if (batch->error)
            std::rethrow_exception(batch->error);
    }

    void WorkerPool::post(std::function<void()> task)
    {
        {
            const std::lock_guard lock(_mutex);
            _queue.emplace_back([task = std::move(task)] {
                const ExecutionContext::Scope scope(std::optional<ExecutionContext>{});
                task();
            });
        }
        _wake.notify_one();
    }

    void WorkerPool::_workerLoop()
    {
        for (;;)
        {
            Task task;
            {
                std::unique_lock lock(_mutex);
                _wake.wait(lock, [this] { return _stopping || !_queue.empty(); });
                if (_queue.empty())
                    return;
                task = std::move(_queue.front());
                _queue.pop_front();
            }
            task();
        }
    }
} // namespace platform::threading
