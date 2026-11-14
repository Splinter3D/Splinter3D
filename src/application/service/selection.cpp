#include "selection.hpp"

#include <algorithm>
#include <utility>

namespace application::service
{
    void Selection::selectOnly(ModelId id)
    {
        if (ids_.size() == 1 && ids_.front() == id)
            return;
        ids_ = {id};
        notify();
    }

    void Selection::set(const std::vector<ModelId>& ids)
    {
        std::vector<ModelId> unique;
        for (const ModelId id : ids)
            if (std::find(unique.begin(), unique.end(), id) == unique.end())
                unique.push_back(id);
        if (unique == ids_)
            return;
        ids_ = std::move(unique);
        notify();
    }

    void Selection::toggle(ModelId id)
    {
        if (!remove(id))
        {
            ids_.push_back(id);
            notify();
        }
    }

    void Selection::clear()
    {
        if (ids_.empty())
            return;
        ids_.clear();
        notify();
    }

    bool Selection::remove(ModelId id)
    {
        const auto found = std::find(ids_.begin(), ids_.end(), id);
        if (found == ids_.end())
            return false;
        ids_.erase(found);
        notify();
        return true;
    }

    void Selection::retainOnly(const std::vector<ModelId>& existing)
    {
        const auto gone = std::remove_if(ids_.begin(), ids_.end(), [&](ModelId id) {
            return std::find(existing.begin(), existing.end(), id) == existing.end();
        });
        if (gone == ids_.end())
            return;
        ids_.erase(gone, ids_.end());
        notify();
    }

    bool Selection::contains(ModelId id) const noexcept
    {
        return std::find(ids_.begin(), ids_.end(), id) != ids_.end();
    }

    const std::vector<ModelId>& Selection::ids() const noexcept
    {
        return ids_;
    }

    std::size_t Selection::size() const noexcept
    {
        return ids_.size();
    }

    bool Selection::empty() const noexcept
    {
        return ids_.empty();
    }

    void Selection::setOnChanged(std::function<void()> callback)
    {
        onChanged_ = std::move(callback);
    }

    void Selection::notify() const
    {
        if (onChanged_)
            onChanged_();
    }
} // namespace application::service
