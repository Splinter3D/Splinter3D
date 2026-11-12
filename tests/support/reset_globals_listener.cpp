#include "core/event_bus/event_bus.hpp"
#include "platform/locale/locale_manager.hpp"
#include "platform/settings/settings_registry.hpp"
#include "platform/threading/dispatcher.hpp"

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
            platform::threading::Dispatcher::getInstance().clear();
            // Tests learn from instant runs; the one testing the threshold sets it.
            platform::threading::Dispatcher::getInstance().setMinLearnedDuration(platform::benchmark::Duration::zero());
            // UI strings come from assets/locale/*.json; tests assert the English ones.
            platform::locale::LocaleManager::get().init(SPLINTER3D_TEST_LOCALE_DIR);
            platform::locale::LocaleManager::get().setLanguage("en");
            // Defaults only, and never the user's settings file.
            platform::settings::SettingsRegistry::getInstance().clear();
        }

        void testCaseEnded(const Catch::TestCaseStats&) override
        {
            core::event_bus::EventBus::getInstance().clear();
            platform::threading::Dispatcher::getInstance().clear();
        }
    };
} // namespace

CATCH_REGISTER_LISTENER(ResetGlobalsListener)
