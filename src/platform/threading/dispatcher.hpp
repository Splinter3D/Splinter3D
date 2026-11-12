#pragma once

#include "core/utils/singleton.hpp"
#include "execution_context.hpp"
#include "platform/benchmark/benchmark.hpp"
#include "scalability_profile.hpp"
#include "worker_pool.hpp"

#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace platform::threading
{
    /**
     * @brief Shares the machine's cores between expensive operations.
     *
     * run() gives an operation a worker budget, sized from its
     * ScalabilityProfile (or a fair share of the free workers while it has
     * none) and capped by what other running operations hold. Code inside
     * reads the budget with NPROC and spreads work with parallelFor() over
     * one global WorkerPool. Each run is measured with benchmark::measure()
     * and fed back into the operation's profile; load()/save() keep the
     * profiles between launches, in <data root>/assets/kProfilesFile.
     *
     * @code{.cpp}
     * auto result = Dispatcher::getInstance().run("boolean.cut", [&] { return cutter.perform(); });
     * // deep inside: Dispatcher::getInstance().parallelFor(0, faces.size(), [&](std::size_t i) { ... });
     * @endcode
     */
    class Dispatcher final : public core::utils::Singleton<Dispatcher>
    {
      public:
        /** @brief Name of the profiles file, kept in the assets folder (git-ignored). */
        static constexpr std::string_view kProfilesFile = "splinter3D.profiles.json";

        /**
         * @brief Runs shorter than this are not learned from: their CPU time
         * is mostly measuring overhead, not how the work scales.
         */
        static constexpr benchmark::Duration kMinLearnedDuration{20.0};

        /**
         * @brief Where the profiles live: $SPLINTER3D_PROFILES if set (tests
         * point it at a temporary file), else <dataRoot>/assets/kProfilesFile.
         */
        [[nodiscard]] static std::filesystem::path profilesPath(const std::filesystem::path& dataRoot);

        /** @brief Every kExploreEvery-th run of an operation tries one worker more or fewer. */
        static constexpr std::size_t kExploreEvery = 8;

        /**
         * @brief Runs fn() under a worker budget chosen for `operation` and
         * returns its result. The run is recorded in the operation's profile
         * unless fn throws; the budget is released either way. A run() inside
         * another one shares (never exceeds) the outer budget.
         */
        template <typename Func>
        decltype(auto) run(std::string_view operation, Func&& fn)
        {
            const Lease lease(*this, operation);
            return benchmark::measure(&lease.report, kMetrics, std::forward<Func>(fn));
        }

        /**
         * @brief Runs run(operation, fn) on a pool thread and returns at once.
         * The pool thread counts as the operation's calling thread, so NPROC
         * and parallelFor() inside fn work as in run(). fn should not throw;
         * if it does, the exception is logged and dropped. then, if given,
         * runs on the same thread once the run's budget is released (also
         * after fn threw); it must not throw.
         */
        void submit(std::string operation, std::function<void()> fn, std::function<void()> then = {});

        /**
         * @brief Runs fn on a pool thread outside any operation: no budget is
         * taken and nothing is recorded. For housekeeping (e.g. freeing big
         * objects off the UI thread), not for work that should share the
         * CPU fairly. fn should not throw; if it does, it is logged and dropped.
         */
        void post(std::function<void()> fn);

        /** @brief Told that workers became free (see onCapacityFreed()). */
        using CapacityListener = std::function<void()>;

        /**
         * @brief Calls listener whenever a top-level run gives its workers
         * back (nested runs free nothing), on the releasing thread, outside
         * any lock: anything waiting for workers can try again, whoever held
         * them. Keep it short; post to another thread for real work.
         *
         * @return A token for unsubscribe().
         */
        std::uint64_t onCapacityFreed(CapacityListener listener);

        /** @brief Stops calling the listener; once this returns, it is not running any more either. */
        void unsubscribe(std::uint64_t token);

        /** @brief Workers no top-level run holds right now; 0 when saturated. */
        [[nodiscard]] std::size_t freeWorkers() const noexcept;

        /** @brief The current thread's worker budget; 1 outside any run(). */
        [[nodiscard]] std::size_t nproc() const noexcept;

        /** @brief Most workers a budget can hold: the pool plus the calling thread. */
        [[nodiscard]] std::size_t total() const noexcept;

        /** @brief Calls body(i) for each i in [first, last) on nproc() workers of the global pool. */
        void parallelFor(std::size_t first, std::size_t last, const std::function<void(std::size_t)>& body);

        /** @brief The operation's profile; empty until a run of it is recorded. */
        [[nodiscard]] std::optional<ScalabilityProfile> profile(std::string_view operation) const;

        /**
         * @brief Replaces every profile with the file's and remembers the path
         * for save(). A missing file leaves none; an unreadable one is
         * reported on the log and leaves none too. Never throws.
         */
        void load(const std::filesystem::path& path);

        /** @brief save(path) to the path last given to load(); does nothing before any load(). */
        void save() const;

        /**
         * @brief Writes every profile to the file, creating its folder. Writes
         * nothing while no run has been recorded. The file is replaced
         * through a temporary one, so a crash never leaves it half-written;
         * a failure is reported on the log.
         */
        void save(const std::filesystem::path& path) const;

#if defined(SPLINTER3D_TESTING)
        /**
         * @brief Test-only: forgets every operation and the loaded path (see
         * tests/support/reset_globals_listener.cpp).
         */
        void clear();

        /** @brief Test-only: replaces an operation's profile, keeping its run counter. */
        void setProfile(std::string_view operation, ScalabilityProfile replacement);

        /** @brief Test-only: changes kMinLearnedDuration (tests learn from instant runs). */
        void setMinLearnedDuration(benchmark::Duration duration);
#endif

      private:
        static constexpr benchmark::Metrics kMetrics =
            benchmark::Metrics::Time | benchmark::Metrics::CPUTime | benchmark::Metrics::CPUCores |
            benchmark::Metrics::RAM;

        struct Ticket
        {
            std::string operation;
            std::size_t budget    = 1;
            bool        nested    = false;
            bool        contended = false;
        };

        struct Operation
        {
            ScalabilityProfile profile;
            bool               exploreUp = true;
        };

        // Holds a budget and this thread's context for the length of one run().
        class Lease
        {
          public:
            Lease(Dispatcher& dispatcher, std::string_view operation)
                : _dispatcher(dispatcher)
                , _ticket(dispatcher._acquire(operation))
                , _exceptions(std::uncaught_exceptions())
            {
                // The destructor won't run if this throws, so give the budget back here.
                try
                {
                    _scope.emplace(ExecutionContext{_ticket.operation, _ticket.budget});
                }
                catch (...)
                {
                    _dispatcher._release(_ticket, std::nullopt, false);
                    throw;
                }
            }

            ~Lease()
            {
                _dispatcher._release(_ticket, report, std::uncaught_exceptions() == _exceptions);
            }

            Lease(const Lease&)            = delete;
            Lease& operator=(const Lease&) = delete;

            mutable std::optional<benchmark::Report> report;

          private:
            Dispatcher&                            _dispatcher;
            Ticket                                 _ticket;
            int                                    _exceptions;
            std::optional<ExecutionContext::Scope> _scope;
        };

        Dispatcher();

        Ticket     _acquire(std::string_view operation);
        void       _release(Ticket& ticket, const std::optional<benchmark::Report>& report, bool succeeded) noexcept;
        Operation& _operation(std::string_view operation);
        void       _capacityFreed() noexcept;

        mutable std::mutex                            _mutex;
        std::map<std::string, Operation, std::less<>> _operations;
        std::size_t                                   _allocated = 0;
        std::size_t                                   _active    = 0;
        std::filesystem::path                         _path;
        benchmark::Duration                           _minLearned = kMinLearnedDuration;
        // Separate from _mutex: listeners run outside it. Held while calling
        // them, so unsubscribe() returns only once its listener is done.
        std::mutex                                _listenersMutex;
        std::map<std::uint64_t, CapacityListener> _listeners;
        std::uint64_t                             _nextToken = 1;
        WorkerPool                                _pool;

        friend class core::utils::Singleton<Dispatcher>;
    };
} // namespace platform::threading

/** @brief The worker budget of the operation the calling thread works for (1 outside any). */
#define NPROC (::platform::threading::Dispatcher::getInstance().nproc())
