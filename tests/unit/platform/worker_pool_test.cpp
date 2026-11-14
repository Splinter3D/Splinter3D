#include "platform/threading/execution_context.hpp"
#include "platform/threading/worker_pool.hpp"

#include <array>
#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <cstddef>
#include <latch>
#include <mutex>
#include <set>
#include <stdexcept>
#include <thread>
#include <vector>

using platform::threading::ExecutionContext;
using platform::threading::WorkerPool;

TEST_CASE("WorkerPool: visits every index exactly once", "[platform][threading]")
{
    WorkerPool                    pool(3);
    std::vector<std::atomic<int>> visits(10000);
    pool.parallelFor(0, visits.size(), 4, [&](std::size_t i) { ++visits[i]; });
    for (const auto& count : visits)
        REQUIRE(count == 1);
}

TEST_CASE("WorkerPool: empty or reversed range is a no-op", "[platform][threading]")
{
    WorkerPool       pool(2);
    std::atomic<int> calls = 0;
    pool.parallelFor(5, 5, 4, [&](std::size_t) { ++calls; });
    pool.parallelFor(7, 3, 4, [&](std::size_t) { ++calls; });
    REQUIRE(calls == 0);
}

TEST_CASE("WorkerPool: never uses more than maxWorkers threads", "[platform][threading]")
{
    WorkerPool                pool(7);
    std::mutex                mutex;
    std::set<std::thread::id> threads;
    pool.parallelFor(0, 64, 2, [&](std::size_t) {
        {
            const std::lock_guard lock(mutex);
            threads.insert(std::this_thread::get_id());
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    });
    REQUIRE(threads.size() <= 2);
}

TEST_CASE("WorkerPool: nested parallelFor completes", "[platform][threading]")
{
    WorkerPool       pool(2);
    std::atomic<int> calls = 0;
    pool.parallelFor(0, 8, 3, [&](std::size_t) { pool.parallelFor(0, 8, 3, [&](std::size_t) { ++calls; }); });
    REQUIRE(calls == 64);
}

TEST_CASE("WorkerPool: exception propagates and pool stays usable", "[platform][threading]")
{
    WorkerPool pool(3);

    // The caller holds its first slice until a pool thread has thrown, so the
    // throw always comes from a pool thread.
    const auto        caller = std::this_thread::get_id();
    std::atomic<bool> thrown = false;
    REQUIRE_THROWS_AS(pool.parallelFor(0, 16, 4,
                                       [&](std::size_t) {
                                           if (std::this_thread::get_id() == caller)
                                           {
                                               while (!thrown)
                                                   std::this_thread::yield();
                                               return;
                                           }
                                           thrown = true;
                                           throw std::runtime_error("boom");
                                       }),
                      std::runtime_error);

    std::atomic<int> calls = 0;
    pool.parallelFor(0, 100, 4, [&](std::size_t) { ++calls; });
    REQUIRE(calls == 100);
}

TEST_CASE("WorkerPool: each slice gets its share of the submitter's budget", "[platform][threading]")
{
    WorkerPool                    pool(3);
    const ExecutionContext::Scope scope({"op", 4});

    // 64 indices on 4 workers: 4 slices, one worker each.
    std::atomic<int> mismatches = 0;
    pool.parallelFor(0, 64, 4, [&](std::size_t) {
        const auto& context = ExecutionContext::current();
        if (!context || context->operation != "op" || context->budget != 1)
            ++mismatches;
    });
    REQUIRE(mismatches == 0);

    // 2 indices on 4 workers: 2 slices, two workers each, so nested work stays within 4.
    std::array<std::size_t, 2> shares{};
    pool.parallelFor(0, 2, 4, [&](std::size_t i) { shares[i] = ExecutionContext::current()->budget; });
    REQUIRE(shares[0] == 2);
    REQUIRE(shares[1] == 2);
}

TEST_CASE("WorkerPool: tasks of a caller without context see none", "[platform][threading]")
{
    WorkerPool       pool(3);
    std::atomic<int> withContext = 0;
    pool.parallelFor(0, 64, 4, [&](std::size_t) {
        if (ExecutionContext::current())
            ++withContext;
    });
    REQUIRE(withContext == 0);
}

TEST_CASE("WorkerPool: a waiting caller never runs another call's slices", "[platform][threading]")
{
    WorkerPool pool(1);

    // Park the only worker inside slice 1 of a first call, whose caller stays in slice 0.
    std::latch  workerParked(1);
    std::latch  release(1);
    std::thread first([&] {
        pool.parallelFor(0, 2, 2, [&](std::size_t i) {
            if (i == 1)
                workerParked.count_down();
            release.wait();
        });
    });
    workerParked.wait();

    // A second call queues its slice 1, then holds its caller in slice 0 until the main thread is done.
    std::latch      secondQueued(1);
    std::latch      mainDone(1);
    std::thread::id secondSliceThread;
    std::thread     second([&] {
        pool.parallelFor(0, 2, 2, [&](std::size_t i) {
            if (i == 1)
                secondSliceThread = std::this_thread::get_id();
            else
            {
                secondQueued.count_down();
                mainDone.wait();
            }
        });
    });
    secondQueued.wait();

    // The main thread's own call must finish without picking up the second call's queued slice.
    pool.parallelFor(0, 2, 2, [](std::size_t) { });
    mainDone.count_down();
    second.join();
    release.count_down();
    first.join();

    REQUIRE(secondSliceThread != std::this_thread::get_id());
}

TEST_CASE("WorkerPool: post runs the task on a pool thread without context", "[platform][threading]")
{
    WorkerPool                    pool(2);
    const ExecutionContext::Scope scope({"caller", 2});
    std::latch                    ran(1);
    std::thread::id               thread;
    bool                          hadContext = true;
    pool.post([&] {
        thread     = std::this_thread::get_id();
        hadContext = ExecutionContext::current().has_value();
        ran.count_down();
    });
    ran.wait();
    REQUIRE(thread != std::this_thread::get_id());
    REQUIRE_FALSE(hadContext);
}
