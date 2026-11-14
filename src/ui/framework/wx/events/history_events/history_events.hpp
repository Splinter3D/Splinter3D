#pragma once

#include "../event_binder.hpp"

#include <functional>
#include <utility>

namespace ui::framework::wx
{
    namespace events
    {
        /**
         * @brief Undo and redo: the Edit menu's items, the toolbar's buttons
         * and their shortcuts (ids::history), each enabled only while there
         * is something to undo or redo.
         */
        class HistoryEvents final : public EventBinder
        {
          public:
            using Action    = std::function<void()>;
            using Predicate = std::function<bool()>;

            HistoryEvents(wxFrame* frame, Action on_undo, Action on_redo, Predicate can_undo, Predicate can_redo)
                : EventBinder(frame), on_undo_(std::move(on_undo)), on_redo_(std::move(on_redo)), can_undo_(std::move(can_undo)), can_redo_(std::move(can_redo))
            {
            }

            void bind() override;

          private:
            void bindAction(int id, const Action& action, const Predicate& enabled);

            Action    on_undo_;
            Action    on_redo_;
            Predicate can_undo_;
            Predicate can_redo_;
        };

    } // namespace events
} // namespace ui::framework::wx
