#pragma once

#include <cstddef>
#include <optional>
#include <string>

namespace platform::threading
{
    /**
     * @brief The operation a thread is currently working for, and how many
     * workers that operation may use.
     *
     * Each thread has its own (empty outside any Dispatcher::run()). Tasks
     * the WorkerPool runs carry their submitter's, so code nested inside an
     * operation sees that operation's budget on every thread.
     */
    struct ExecutionContext
    {
        std::string operation;
        std::size_t budget = 1;

        /** @brief This thread's context, empty outside any Scope. */
        [[nodiscard]] static const std::optional<ExecutionContext>& current() noexcept;

        class Scope;
    };

    /**
     * @brief Installs a context on this thread while alive (an empty one
     * clears it), then restores the previous one.
     */
    class ExecutionContext::Scope
    {
      public:
        explicit Scope(ExecutionContext context);
        explicit Scope(std::optional<ExecutionContext> context);
        ~Scope();

        Scope(const Scope&)            = delete;
        Scope& operator=(const Scope&) = delete;

      private:
        std::optional<ExecutionContext> _previous;
    };
} // namespace platform::threading
