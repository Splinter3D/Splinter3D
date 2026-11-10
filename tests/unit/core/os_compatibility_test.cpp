#include "core/utils/os_compatibility.hpp"

#include <catch2/catch_test_macros.hpp>
#include <csignal>

TEST_CASE("Signal flag: set by handled signals, reset by ClearSignal()", "[core][signals]")
{
    core::utils::ClearSignal();
    REQUIRE_FALSE(core::utils::SignalReceived());

#if defined(SPLINTER3D_POSIX)
    core::utils::InstallSignalHandlers();

    for (int signal : {SIGINT, SIGTERM})
    {
        CAPTURE(signal);
        std::raise(signal);
        CHECK(core::utils::SignalReceived());
        core::utils::ClearSignal();
        CHECK_FALSE(core::utils::SignalReceived());
    }

    // Give the test process its default behaviour back.
    std::signal(SIGINT, SIG_DFL);
    std::signal(SIGTERM, SIG_DFL);
#else
    // Handlers are a no-op on Windows; only the flag API exists.
    core::utils::InstallSignalHandlers();
    CHECK_FALSE(core::utils::SignalReceived());
#endif
}

TEST_CASE("Terminal echo helpers are safe without a terminal", "[core][signals]")
{
    // Under the test runner stdin is usually not a TTY: both must be no-ops then.
    core::utils::disableCtrlCEcho();
    core::utils::restoreTerminal();
    SUCCEED();
}
