#include "application/tasks/task_context.hpp"

#include <Message_ProgressScope.hxx>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <stdexcept>

using application::tasks::TaskContext;
using Catch::Matchers::WithinAbs;

TEST_CASE("TaskContext: progress is clamped and readable", "[application][tasks]")
{
    TaskContext context;
    CHECK_FALSE(context.progress());
    context.setProgress(0.25);
    REQUIRE(context.progress());
    CHECK_THAT(*context.progress(), WithinAbs(0.25, 1e-12));
    context.setProgress(3.0);
    CHECK_THAT(*context.progress(), WithinAbs(1.0, 1e-12));
    context.setProgress(-1.0);
    CHECK_THAT(*context.progress(), WithinAbs(0.0, 1e-12));
}

TEST_CASE("TaskContext: indeterminate clears progress and keeps status", "[application][tasks]")
{
    TaskContext context;
    context.setProgress(0.5);
    context.setIndeterminate("Writing…");
    CHECK_FALSE(context.progress());
    CHECK(context.status() == "Writing…");
}

TEST_CASE("TaskContext: progressRange forwards OCCT progress", "[application][tasks]")
{
    TaskContext context;
    {
        Message_ProgressScope scope(context.progressRange(), "steps", 4);
        scope.Next();
        scope.Next();
        REQUIRE(context.progress());
        CHECK_THAT(*context.progress(), WithinAbs(0.5, 1e-9));
    }
    REQUIRE_THROWS_AS(context.progressRange(), std::logic_error);
}

TEST_CASE("TaskContext: cancel makes the range report a user break", "[application][tasks]")
{
    TaskContext                 context;
    const Message_ProgressScope scope(context.progressRange(), "steps", 4);
    CHECK_FALSE(context.cancelled());
    CHECK_FALSE(scope.UserBreak());
    context.cancel();
    CHECK(context.cancelled());
    CHECK(scope.UserBreak());
}

TEST_CASE("TaskContext: onChange fires on progress and status updates", "[application][tasks]")
{
    TaskContext context;
    int         changes = 0;
    context.setOnChange([&] { ++changes; });
    context.setProgress(0.1);
    context.setIndeterminate("Writing…");
    CHECK(changes == 2);
}
