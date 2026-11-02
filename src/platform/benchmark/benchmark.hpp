
#pragma once

#include <chrono>
#include <concepts>
#include <core/utils/non_copyable.hpp>
#include <functional>
#include <optional>
#include <type_traits>
#include <utility>

namespace platform::benchmark
{
    namespace detail
    {
        template <std::invocable F>
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

    /**
     * @brief Wall-clock timer (steady clock, milliseconds) for measuring how
     * long an operation takes.
     *
     * Either bracket the work with start()/stop(), or wrap a call in measure(),
     * which also records the duration if the call throws.
     *
     * @code{.cpp}
     * platform::benchmark::Benchmark benchmark;
     * std::optional<platform::benchmark::Benchmark::Duration> duration;
     * auto shape = benchmark.measure(&duration, &geometry::occt::Shape::fromSTL, path);
     * // duration->count() is the call's duration in milliseconds
     * @endcode
     */
    class Benchmark final : public core::utils::NonCopyable
    {
      public:
        constexpr Benchmark() noexcept = default;
        ~Benchmark() noexcept          = default;

        using Duration = std::chrono::duration<double, std::milli>;

        void start() noexcept
        {
            _startTime = Clock::now();
            _endTime   = _startTime;
        }

        [[nodiscard]] Duration elapsed() const noexcept
        {
            return Clock::now() - _startTime;
        }

        [[nodiscard]] Duration stop() noexcept
        {
            _endTime = Clock::now();
            _lastRun = _endTime - _startTime;
            return _lastRun;
        }

        void reset() noexcept
        {
            start();
            _lastRun = Duration::zero();
        }

        [[nodiscard]] Duration lastDuration() const noexcept
        {
            return _lastRun;
        }

        template <typename Func, typename... Args>
            requires std::invocable<Func, Args...>
        auto measure(
            std::optional<Duration>* duration,
            Func&&                   func,
            Args&&... args) noexcept(noexcept(std::invoke(std::forward<Func>(func),
                                                          std::forward<Args>(args)...)))
        {
            const auto startTime = Clock::now();

            const detail::ScopeExit guard([&]() noexcept {
                if (duration)
                    *duration = Clock::now() - startTime;
            });

            return std::invoke(
                std::forward<Func>(func),
                std::forward<Args>(args)...);
        }

      private:
        using Clock     = std::chrono::steady_clock;
        using TimePoint = Clock::time_point;

        TimePoint _startTime{};
        TimePoint _endTime{};
        Duration  _lastRun{};
    };
} // namespace platform::benchmark
