#include "platform/benchmark/benchmark.hpp"

#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <optional>
#include <stdexcept>
#include <thread>
#include <type_traits>

using platform::benchmark::Benchmark;
using namespace std::chrono_literals;

namespace
{
    constexpr auto kSleep = 20ms;

    void sleepFor(std::chrono::milliseconds duration)
    {
        std::this_thread::sleep_for(duration);
    }

    int add(int a, int b) noexcept
    {
        return a + b;
    }
} // namespace

static_assert(!std::is_copy_constructible_v<Benchmark>);
static_assert(!std::is_move_constructible_v<Benchmark>);
static_assert(std::is_same_v<Benchmark::Duration::period, std::milli>);

TEST_CASE("Benchmark: a fresh benchmark has no last duration", "[platform][benchmark]")
{
    const Benchmark benchmark;
    REQUIRE(benchmark.lastDuration() == Benchmark::Duration::zero());
}

TEST_CASE("Benchmark: start/stop measures the time in between", "[platform][benchmark]")
{
    Benchmark benchmark;
    benchmark.start();
    sleepFor(kSleep);

    const auto stopped = benchmark.stop();

    // Sleeps last at least as long as asked; no upper bound (CI machines stall).
    CHECK(stopped >= kSleep);
    CHECK(benchmark.lastDuration() == stopped);
}

TEST_CASE("Benchmark: elapsed() reads the running time without stopping", "[platform][benchmark]")
{
    Benchmark benchmark;
    benchmark.start();
    sleepFor(kSleep);

    const auto first = benchmark.elapsed();
    CHECK(first >= kSleep);
    CHECK(benchmark.lastDuration() == Benchmark::Duration::zero());

    sleepFor(kSleep);
    CHECK(benchmark.elapsed() >= first + kSleep);
}

TEST_CASE("Benchmark: reset() restarts and forgets the last duration", "[platform][benchmark]")
{
    Benchmark benchmark;
    benchmark.start();
    sleepFor(kSleep);
    (void) benchmark.stop();

    benchmark.reset();

    CHECK(benchmark.lastDuration() == Benchmark::Duration::zero());
    CHECK(benchmark.elapsed() < kSleep * 50); // restarted, not counting from the first start()
}

TEST_CASE("Benchmark: measure() returns the result and records the duration", "[platform][benchmark]")
{
    Benchmark                          benchmark;
    std::optional<Benchmark::Duration> duration;

    const int result = benchmark.measure(&duration, add, 2, 3);

    CHECK(result == 5);
    REQUIRE(duration.has_value());
    CHECK(*duration >= Benchmark::Duration::zero());
    // measure() does not touch start/stop state.
    CHECK(benchmark.lastDuration() == Benchmark::Duration::zero());
}

TEST_CASE("Benchmark: measure() works with void callables and lambdas", "[platform][benchmark]")
{
    Benchmark                          benchmark;
    std::optional<Benchmark::Duration> duration;
    int                                calls = 0;

    benchmark.measure(&duration, [&calls] {
        ++calls;
        sleepFor(kSleep);
    });

    CHECK(calls == 1);
    REQUIRE(duration.has_value());
    CHECK(*duration >= kSleep);
}

TEST_CASE("Benchmark: measure() still records the duration when the call throws", "[platform][benchmark]")
{
    Benchmark                          benchmark;
    std::optional<Benchmark::Duration> duration;

    CHECK_THROWS_AS(benchmark.measure(&duration,
                                      [] {
                                          sleepFor(kSleep);
                                          throw std::runtime_error("boom");
                                      }),
                    std::runtime_error);

    REQUIRE(duration.has_value());
    CHECK(*duration >= kSleep);
}

TEST_CASE("Benchmark: measure() accepts a null duration", "[platform][benchmark]")
{
    Benchmark benchmark;
    CHECK(benchmark.measure(nullptr, add, 1, 1) == 2);
}

TEST_CASE("Benchmark: measure() forwards noexcept", "[platform][benchmark]")
{
    Benchmark                          benchmark;
    std::optional<Benchmark::Duration> duration;

    STATIC_REQUIRE(noexcept(benchmark.measure(&duration, add, 1, 2)));
    STATIC_REQUIRE_FALSE(noexcept(benchmark.measure(&duration, [] { throw 1; })));
}
