#include "history.hpp"

#include <utility>

namespace application::commands
{
    namespace
    {
        History::State withoutBReps(History::State state)
        {
            for (auto& model : state.models)
                model.brep.reset();
            state.visibility.resize(state.models.size(), true);
            return state;
        }
    } // namespace

    void History::record(State before, std::string mergeKey, Clock::time_point now)
    {
        const bool merges = !mergeKey.empty() && mergeKey == mergeKey_ && !undo_.empty() && now - lastRecord_ < kMergeWindow;
        mergeKey_         = std::move(mergeKey);
        lastRecord_       = now;
        redo_.clear();
        if (merges)
            return;
        undo_.push_back(withoutBReps(std::move(before)));
        if (undo_.size() > kLimit)
            undo_.pop_front();
    }

    bool History::canUndo() const noexcept
    {
        return !undo_.empty();
    }

    bool History::canRedo() const noexcept
    {
        return !redo_.empty();
    }

    std::optional<History::State> History::undo(State current)
    {
        if (undo_.empty())
            return std::nullopt;
        mergeKey_.clear();
        State previous = std::move(undo_.back());
        undo_.pop_back();
        redo_.push_back(withoutBReps(std::move(current)));
        return previous;
    }

    std::optional<History::State> History::redo(State current)
    {
        if (redo_.empty())
            return std::nullopt;
        mergeKey_.clear();
        State next = std::move(redo_.back());
        redo_.pop_back();
        undo_.push_back(withoutBReps(std::move(current)));
        return next;
    }

    void History::clear() noexcept
    {
        undo_.clear();
        redo_.clear();
        mergeKey_.clear();
    }
} // namespace application::commands
