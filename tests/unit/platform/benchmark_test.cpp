#include "platform/benchmark/benchmark.hpp"

#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <memory>
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

namespace
{
    // Copying throws: returning it by value from a noexcept measure() would terminate.
    struct ThrowingCopy
    {
        int value = 0;

        ThrowingCopy() = default;

        ThrowingCopy(const ThrowingCopy&) noexcept(false)
        { }

        ThrowingCopy& operator=(const ThrowingCopy&) noexcept(false)
        {
            return *this;
        }
    };

    ThrowingCopy g_shared;

    ThrowingCopy& sharedInstance() noexcept
    {
        return g_shared;
    }

    template <typename F>
    concept GuardableBy = requires { typename platform::benchmark::detail::ScopeExit<F>; };

    // Only their declarations matter (checked in unevaluated contexts).
    struct NothrowCallback
    {
        void operator()() const noexcept;
    };

    struct ThrowingCallback
    {
        void operator()() const;
    };
} // namespace

TEST_CASE("Benchmark: measure() returns references as references", "[platform][benchmark]")
{
    Benchmark                          benchmark;
    std::optional<Benchmark::Duration> duration;
    int                                value = 1;

    int& ref = benchmark.measure(&duration, [&value]() noexcept -> int& { return value; });
    ref      = 42;

    CHECK(value == 42);
    CHECK(duration.has_value());

    const int& constRef = benchmark.measure(&duration, [&value]() -> const int& { return value; });
    CHECK(&constRef == &value);

    STATIC_REQUIRE(std::is_same_v<decltype(benchmark.measure(&duration, [&value]() -> int& { return value; })), int&>);
    STATIC_REQUIRE(
        std::is_same_v<decltype(benchmark.measure(&duration, [&value]() -> const int& { return value; })), const int&>);
}

TEST_CASE("Benchmark: measure() never copies a returned reference", "[platform][benchmark]")
{
    Benchmark                          benchmark;
    std::optional<Benchmark::Duration> duration;

    // noexcept is forwarded from sharedInstance(), and nothing is copied, so the
    // throwing copy constructor can never run inside it.
    STATIC_REQUIRE(noexcept(benchmark.measure(&duration, sharedInstance)));
    STATIC_REQUIRE(std::is_same_v<decltype(benchmark.measure(&duration, sharedInstance)), ThrowingCopy&>);

    ThrowingCopy& result = benchmark.measure(&duration, sharedInstance);
    CHECK(&result == &g_shared);
}

TEST_CASE("Benchmark: measure() returns values and move-only results by value", "[platform][benchmark]")
{
    Benchmark                          benchmark;
    std::optional<Benchmark::Duration> duration;

    STATIC_REQUIRE(std::is_same_v<decltype(benchmark.measure(&duration, add, 1, 2)), int>);
    STATIC_REQUIRE(std::is_same_v<decltype(benchmark.measure(&duration, [] { })), void>);

    auto owned = benchmark.measure(&duration, [] { return std::make_unique<int>(7); });
    STATIC_REQUIRE(std::is_same_v<decltype(owned), std::unique_ptr<int>>);
    REQUIRE(owned != nullptr);
    CHECK(*owned == 7);
}

TEST_CASE("ScopeExit: runs its callback when leaving the scope", "[platform][benchmark]")
{
    int calls = 0;
    {
        const platform::benchmark::detail::ScopeExit guard([&calls]() noexcept { ++calls; });
        CHECK(calls == 0);
    }
    CHECK(calls == 1);

    try
    {
        const platform::benchmark::detail::ScopeExit guard([&calls]() noexcept { ++calls; });
        throw std::runtime_error("unwind");
    }
    catch (const std::runtime_error&)
    {
    }
    CHECK(calls == 2);
}

// Only callables that cannot throw are accepted, since the destructor is noexcept.
static_assert(GuardableBy<NothrowCallback>);
static_assert(!GuardableBy<ThrowingCallback>);
static_assert(!GuardableBy<int>);
static_assert(!std::is_copy_constructible_v<platform::benchmark::detail::ScopeExit<NothrowCallback>>);
