#pragma once

#include "../event_binder.hpp"

#include <functional>

namespace ui::framework::wx
{
    namespace events
    {
        /**
         * @brief Handles application language switching events.
         */
        class LocaleEvents final : public EventBinder
        {

        public:
            using EventBinder::EventBinder;

            /**
             * @brief Registers locale events.
             */
            void bind() override;

            /**
             * @brief Sets callback called after language change.
             */
            void setLanguageCallback(
                std::function<void(const std::string &)> callback);

        private:
            void bindLanguages();

        private:
            std::function<void(const std::string &)> on_language_changed_;
        };

    } // namespace events
} // namespace ui::framework::wx
