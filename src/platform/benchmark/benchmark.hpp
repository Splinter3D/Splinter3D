
#pragma once

#include <chrono>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iomanip>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>

namespace platform::benchmark
{
    namespace detail
    {
        template <typename F>
            requires std::is_nothrow_invocable_v<F&>
        class ScopeExit
        {
          public:
            explicit ScopeExit(F func) noexcept(std::is_nothrow_move_constructible_v<F>)
                : _func(std::move(func))
            { }

            ~ScopeExit() noexcept
            {
                _func();
            }

            ScopeExit(const ScopeExit&)            = delete;
            ScopeExit& operator=(const ScopeExit&) = delete;

          private:
            F _func;
        };
    } // namespace detail

    enum Metrics
    {
        CPUTime  = 1 << 0,
        CPUCores = 1 << 1,
        RAM      = 1 << 2,
        GPU      = 1 << 3,
        Time     = 1 << 4,
        FPS      = 1 << 5,
        ALL      = CPUTime | CPUCores | RAM | GPU | Time | FPS,
        NOFPS    = CPUTime | CPUCores | RAM | GPU | Time
    };

    [[nodiscard]] constexpr Metrics operator|(Metrics lhs, Metrics rhs) noexcept
    {
        return static_cast<Metrics>(static_cast<int>(lhs) | static_cast<int>(rhs));
    }

    [[nodiscard]] constexpr Metrics operator&(Metrics lhs, Metrics rhs) noexcept
    {
        return static_cast<Metrics>(static_cast<int>(lhs) & static_cast<int>(rhs));
    }

    /**
     * @brief UI heartbeat, which measure() turns into Report::longestFrame to
     * tell whether the UI froze during a call.
     *
     * The UI thread calls tick() once per event-loop frame (app::WxApp does it
     * from a ~60 Hz timer). Timers only fire while the event loop runs, so a
     * blocked UI thread stops ticking and longestFrame grows to the length of
     * the freeze. Lock-free: tick() from a single
     * thread, read from any.
     */
    class FrameClock final
    {
      public:
        using Clock = std::chrono::steady_clock;

        /** @brief Frames between two instants, and the longest wait for one. */
        struct Window
        {
            std::size_t              frames = 0;
            std::chrono::nanoseconds longestGap{};
        };

        FrameClock() = delete;

        /** @brief Records a frame now. Call it from the UI thread only. */
        static void tick() noexcept;

        /** @brief Whether tick() was ever called, i.e. a UI is running. */
        [[nodiscard]] static bool running() noexcept;

        /**
         * @brief Counts the frames ticked in [start, end] and the longest
         * interval without one, edges included: a window with no frame at all
         * has longestGap == end - start. Only the last few thousand frames are
         * kept, so a very long window only sees its most recent part.
         */
        [[nodiscard]] static Window between(Clock::time_point start, Clock::time_point end) noexcept;
    };

    using Duration = std::chrono::duration<double, std::milli>;

    /**
     * @brief What measure() sampled. A field is set only when its metric was
     * requested and the platform can provide it.
     */
    struct Report
    {
        std::optional<Duration>     time;
        std::optional<Duration>     cpuTime;
        std::optional<double>       cpuCores;
        std::optional<std::int64_t> ramBytes;
        std::optional<double>       gpu;
        std::optional<Duration>     longestFrame;

        bool operator==(const Report&) const = default;
    };

    namespace detail
    {
        using Clock = FrameClock::Clock;

        struct Sample
        {
            Clock::time_point           wall{};
            std::optional<Duration>     cpuTime;
            std::optional<std::int64_t> ramBytes;
        };

        // Implemented per platform in benchmark.cpp; empty when unavailable.
        [[nodiscard]] std::optional<Duration>     processCpuTime() noexcept;
        [[nodiscard]] std::optional<std::int64_t> residentMemoryBytes() noexcept;

        [[nodiscard]] constexpr bool wants(Metrics metrics, Metrics metric) noexcept
        {
            return (metrics & metric) != 0;
        }

        [[nodiscard]] inline Sample sample(Metrics metrics) noexcept
        {
            Sample result;
            if (wants(metrics, Metrics::CPUTime | Metrics::CPUCores))
                result.cpuTime = processCpuTime();
            if (wants(metrics, Metrics::RAM))
                result.ramBytes = residentMemoryBytes();
            return result;
        }

        [[nodiscard]] inline Report makeReport(Metrics metrics, const Sample& before, const Sample& after) noexcept
        {
            const Duration wall = after.wall - before.wall;

            Report report;
            if (wants(metrics, Metrics::Time))
                report.time = wall;
            if (before.cpuTime && after.cpuTime)
            {
                const Duration cpu = *after.cpuTime - *before.cpuTime;
                if (wants(metrics, Metrics::CPUTime))
                    report.cpuTime = cpu;
                if (wants(metrics, Metrics::CPUCores) && wall > Duration::zero())
                    report.cpuCores = cpu / wall;
            }
            if (wants(metrics, Metrics::RAM) && before.ramBytes && after.ramBytes)
                report.ramBytes = *after.ramBytes - *before.ramBytes;
            if (wants(metrics, Metrics::FPS) && FrameClock::running())
                report.longestFrame = FrameClock::between(before.wall, after.wall).longestGap;
            return report;
        }
    } // namespace detail

    /**
     * @brief Invokes func(args...) and returns its result unchanged, writing
     * what the call cost (the requested metrics, all of them for std::nullopt)
     * to *report, also when the call throws. report may be null.
     *
     * @code{.cpp}
     * std::optional<platform::benchmark::Report> report;
     * auto shape = platform::benchmark::measure(&report, std::nullopt, &geometry::occt::Shape::fromSTL, path);
     * platform::logger::clog("loaded (", *report, ")"); // "time 12.30 ms, cpu 11.80 ms, ..."
     * @endcode
     */
    template <typename Func, typename... Args>
        requires std::invocable<Func, Args...>
    decltype(auto) measure(
        std::optional<Report>* report,
        std::optional<Metrics> metrics,
        Func&&                 func,
        Args&&... args) noexcept(noexcept(std::invoke(std::forward<Func>(func), std::forward<Args>(args)...)))
    {
        const Metrics  wanted = metrics.value_or(Metrics::ALL);
        detail::Sample before = report ? detail::sample(wanted) : detail::Sample{};
        before.wall           = detail::Clock::now(); // after sampling, so it is not counted in the call's time

        const detail::ScopeExit guard([&]() noexcept {
            if (!report)
                return;
            const auto     end   = detail::Clock::now(); // before sampling, for the same reason
            detail::Sample after = detail::sample(wanted);
            after.wall           = end;
            *report              = detail::makeReport(wanted, before, after);
        });

        return std::invoke(std::forward<Func>(func), std::forward<Args>(args)...);
    }

    /**
     * @brief Formats a duration for logs, e.g. "1.54 ms" (two decimals).
     *
     * Uses iostreams rather than std::format, which libstdc++ only provides
     * from GCC 13 (Ubuntu 22.04 ships GCC 11).
     */
    [[nodiscard]] inline std::string formatMilliseconds(Duration duration)
    {
        std::ostringstream stream;
        stream << std::fixed << std::setprecision(2) << duration.count() << " ms";
        return stream.str();
    }

    /**
     * @brief Writes the report's set fields on one line, e.g.
     * "time 1.54 ms, cpu 1.50 ms, cores 0.97, ram +1.20 MiB,
     * longest frame 33.10 ms",
     * or "no metrics" when none is set.
     */
    std::ostream& operator<<(std::ostream& stream, const Report& report);

    /** @brief The report as operator<< writes it. */
    [[nodiscard]] std::string formatReport(const Report& report);
} // namespace platform::benchmark
