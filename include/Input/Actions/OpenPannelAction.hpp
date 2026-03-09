#pragma once

#include <Input/Actions/IAction.hpp>
#include <Splinter3D/Events/EventBus.hpp>
#include <Splinter3D/Events/OpenPannelEvent.hpp>

namespace input::actions
{
    struct OpenPannelAction : IAction
    {
        explicit OpenPannelAction(std::string pannelId)
            : pannelId_(std::move(pannelId))
        { }

        void execute() override
        {
            splinter3D::events::EventBus::getInstance()
                .publish(splinter3D::events::OpenPannelEvent{pannelId_});
        }

      private:
        std::string pannelId_;
    };
} // namespace input::actions
