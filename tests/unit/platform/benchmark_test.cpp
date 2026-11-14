#include "platform/benchmark/benchmark.hpp"

#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

using platform::benchmark::Duration;
using platform::benchmark::FrameClock;
using platform::benchmark::measure;
using platform::benchmark::Report;
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

static_assert(std::is_same_v<Duration::period, std::milli>);

TEST_CASE("Benchmark: measure() returns the result and records the report", "[platform][benchmark]")
{
    std::optional<Report> report;

    const int result = measure(&report, std::nullopt, add, 2, 3);

    CHECK(result == 5);
    REQUIRE(report.has_value());
    REQUIRE(report->time.has_value());
    CHECK(*report->time >= Duration::zero());
}

TEST_CASE("Benchmark: measure() works with void callables and lambdas", "[platform][benchmark]")
{
    std::optional<Report> report;
    int                   calls = 0;

    measure(&report, std::nullopt, [&calls] {
        ++calls;
        sleepFor(kSleep);
    });

    CHECK(calls == 1);
    REQUIRE(report.has_value());
    REQUIRE(report->time.has_value());
    CHECK(*report->time >= kSleep);
}

TEST_CASE("Benchmark: measure() still records the report when the call throws", "[platform][benchmark]")
{
    std::optional<Report> report;

    CHECK_THROWS_AS(measure(&report, std::nullopt,
                            [] {
                                sleepFor(kSleep);
                                throw std::runtime_error("boom");
                            }),
                    std::runtime_error);

    REQUIRE(report.has_value());
    REQUIRE(report->time.has_value());
    CHECK(*report->time >= kSleep);
}

TEST_CASE("Benchmark: measure() accepts a null report", "[platform][benchmark]")
{
    CHECK(measure(nullptr, std::nullopt, add, 1, 1) == 2);
}

TEST_CASE("Benchmark: measure() forwards noexcept", "[platform][benchmark]")
{
    std::optional<Report> report;

    STATIC_REQUIRE(noexcept(measure(&report, std::nullopt, add, 1, 2)));
    STATIC_REQUIRE_FALSE(noexcept(measure(&report, std::nullopt, [] { throw 1; })));
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
    std::optional<Report> report;
    int                   value = 1;

    int& ref = measure(&report, std::nullopt, [&value]() noexcept -> int& { return value; });
    ref      = 42;

    CHECK(value == 42);
    CHECK(report.has_value());

    const int& constRef = measure(&report, std::nullopt, [&value]() -> const int& { return value; });
    CHECK(&constRef == &value);

    STATIC_REQUIRE(std::is_same_v<decltype(measure(&report, std::nullopt, [&value]() -> int& { return value; })), int&>);
    STATIC_REQUIRE(
        std::is_same_v<decltype(measure(&report, std::nullopt, [&value]() -> const int& { return value; })), const int&>);
}

TEST_CASE("Benchmark: measure() never copies a returned reference", "[platform][benchmark]")
{
    std::optional<Report> report;

    // noexcept is forwarded from sharedInstance(), and nothing is copied, so the
    // throwing copy constructor can never run inside it.
    STATIC_REQUIRE(noexcept(measure(&report, std::nullopt, sharedInstance)));
    STATIC_REQUIRE(std::is_same_v<decltype(measure(&report, std::nullopt, sharedInstance)), ThrowingCopy&>);

    ThrowingCopy& result = measure(&report, std::nullopt, sharedInstance);
    CHECK(&result == &g_shared);
}

TEST_CASE("Benchmark: measure() returns values and move-only results by value", "[platform][benchmark]")
{
    std::optional<Report> report;

    STATIC_REQUIRE(std::is_same_v<decltype(measure(&report, std::nullopt, add, 1, 2)), int>);
    STATIC_REQUIRE(std::is_same_v<decltype(measure(&report, std::nullopt, [] { })), void>);

    auto owned = measure(&report, std::nullopt, [] { return std::make_unique<int>(7); });
    STATIC_REQUIRE(std::is_same_v<decltype(owned), std::unique_ptr<int>>);
    REQUIRE(owned != nullptr);
    CHECK(*owned == 7);
}

TEST_CASE("Benchmark: measure() fills only the requested metrics", "[platform][benchmark]")
{
    std::optional<Report> report;

    FrameClock::tick(); // a UI is running, so only the mask keeps longestFrame out

    measure(&report, platform::benchmark::Time | platform::benchmark::RAM, [] { sleepFor(kSleep); });

    REQUIRE(report.has_value());
    REQUIRE(report->time.has_value());
    CHECK(*report->time >= kSleep);
    CHECK(report->ramBytes.has_value());
    CHECK_FALSE(report->cpuTime.has_value());
    CHECK_FALSE(report->cpuCores.has_value());
    CHECK_FALSE(report->gpu.has_value());
    CHECK_FALSE(report->longestFrame.has_value());
}

TEST_CASE("FrameClock: counts the frames ticked in a window", "[platform][benchmark]")
{
    const auto start = FrameClock::Clock::now();
    for (int frame = 0; frame < 3; ++frame)
    {
        sleepFor(2ms);
        FrameClock::tick();
    }
    sleepFor(kSleep);
    const auto end = FrameClock::Clock::now();

    REQUIRE(FrameClock::running());
    const FrameClock::Window window = FrameClock::between(start, end);
    CHECK(window.frames == 3);
    // The quiet stretch after the last frame is the longest gap.
    CHECK(window.longestGap >= kSleep);
    CHECK(window.longestGap < end - start);
}

TEST_CASE("Benchmark: frames stay short while the UI keeps ticking", "[platform][benchmark]")
{
    // Stand-in UI thread: work measured off it must not stall its frames.
    std::atomic<bool> stop{false};
    std::thread       ui([&stop] {
        while (!stop.load())
        {
            FrameClock::tick();
            sleepFor(1ms);
        }
    });

    std::optional<Report> report;
    measure(&report, platform::benchmark::Time | platform::benchmark::FPS, [] { sleepFor(3 * kSleep); });

    stop = true;
    ui.join();

    REQUIRE(report.has_value());
    REQUIRE(report->longestFrame.has_value());
    CHECK(*report->longestFrame < 3 * kSleep);
}

TEST_CASE("Benchmark: a frozen UI shows as one frame lasting the whole call", "[platform][benchmark]")
{
    FrameClock::tick(); // the UI ran before the call...
    sleepFor(2ms);

    std::optional<Report> report;
    // ...then blocks on it, as when the call runs on the UI thread.
    measure(&report, platform::benchmark::Time | platform::benchmark::FPS, [] { sleepFor(kSleep); });

    REQUIRE(report.has_value());
    REQUIRE(report->time.has_value());
    REQUIRE(report->longestFrame.has_value());
    CHECK(*report->longestFrame >= kSleep);
    CHECK((*report->time - *report->longestFrame) < 1ms);
}

TEST_CASE("Benchmark: measure() samples CPU time and resident memory", "[platform][benchmark]")
{
    std::optional<Report> report;

    measure(&report, platform::benchmark::NOFPS, [] {
        std::vector<char> buffer(8 * 1024 * 1024, 1); // touched, so it is resident
        volatile long     sum = 0;
        for (const char value : buffer)
            sum = sum + value;
    });

    REQUIRE(report.has_value());
    REQUIRE(report->time.has_value());
    REQUIRE(report->cpuTime.has_value());
    REQUIRE(report->cpuCores.has_value());
    CHECK(*report->cpuTime >= Duration::zero());
    CHECK(*report->cpuCores >= 0.0);
    CHECK(report->ramBytes.has_value());
    CHECK_FALSE(report->longestFrame.has_value());
}

TEST_CASE("Benchmark: a report formats its set fields", "[platform][benchmark]")
{
    using platform::benchmark::formatReport;

    CHECK(formatReport({}) == "no metrics");

    Report report;
    report.time         = Duration(1.5);
    report.cpuTime      = Duration(3.0);
    report.cpuCores     = 2.0;
    report.ramBytes     = -3 * 1024 * 1024 / 2;
    report.longestFrame = Duration(33.1);
    CHECK(formatReport(report) == "time 1.50 ms, cpu 3.00 ms, cores 2.00, ram -1.50 MiB, longest frame 33.10 ms");

    // operator<< writes the same text and leaves the stream's formatting alone.
    std::ostringstream stream;
    stream << report << ' ' << 0.125;
    CHECK(stream.str() == formatReport(report) + " 0.125");
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
