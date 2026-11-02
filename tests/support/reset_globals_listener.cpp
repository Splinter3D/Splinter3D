#include "core/event_bus/event_bus.hpp"

#include <catch2/catch_test_case_info.hpp>
#include <catch2/reporters/catch_reporter_event_listener.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>

namespace
{
    // Process-wide singletons outlive individual test cases; reset them around each
    // one so the order Catch2 runs tests in (randomized) cannot change the outcome.
    class ResetGlobalsListener : public Catch::EventListenerBase
    {
      public:
        using Catch::EventListenerBase::EventListenerBase;

        void testCaseStarting(const Catch::TestCaseInfo&) override
        {
            core::event_bus::EventBus::getInstance().clear();
        }

        void testCaseEnded(const Catch::TestCaseStats&) override
        {
            core::event_bus::EventBus::getInstance().clear();
        }
    };
} // namespace

CATCH_REGISTER_LISTENER(ResetGlobalsListener)
