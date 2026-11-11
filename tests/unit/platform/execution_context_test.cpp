#include "platform/threading/execution_context.hpp"

#include <catch2/catch_test_macros.hpp>
#include <thread>

using platform::threading::ExecutionContext;

TEST_CASE("ExecutionContext: empty outside any scope", "[platform][threading]")
{
    REQUIRE_FALSE(ExecutionContext::current());
}

TEST_CASE("ExecutionContext: scope installs and restores, nested", "[platform][threading]")
{
    {
        const ExecutionContext::Scope outer({"a", 4});
        REQUIRE(ExecutionContext::current()->operation == "a");
        REQUIRE(ExecutionContext::current()->budget == 4);
        {
            const ExecutionContext::Scope inner({"b", 2});
            REQUIRE(ExecutionContext::current()->operation == "b");
            REQUIRE(ExecutionContext::current()->budget == 2);
        }
        REQUIRE(ExecutionContext::current()->operation == "a");
        REQUIRE(ExecutionContext::current()->budget == 4);
    }
    REQUIRE_FALSE(ExecutionContext::current());
}

TEST_CASE("ExecutionContext: is per thread", "[platform][threading]")
{
    const ExecutionContext::Scope scope({"a", 4});
    bool                          seen = true;
    std::thread([&] { seen = ExecutionContext::current().has_value(); }).join();
    REQUIRE_FALSE(seen);
}
