#include "execution_context.hpp"

#include <utility>

namespace platform::threading
{
    namespace
    {
        thread_local std::optional<ExecutionContext> currentContext;
    } // namespace

    const std::optional<ExecutionContext>& ExecutionContext::current() noexcept
    {
        return currentContext;
    }

    ExecutionContext::Scope::Scope(ExecutionContext context)
        : Scope(std::optional<ExecutionContext>(std::move(context)))
    { }

    ExecutionContext::Scope::Scope(std::optional<ExecutionContext> context)
        : _previous(std::exchange(currentContext, std::move(context)))
    { }

    ExecutionContext::Scope::~Scope()
    {
        currentContext = std::move(_previous);
    }
} // namespace platform::threading
