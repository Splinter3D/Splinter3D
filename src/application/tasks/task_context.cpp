#include "task_context.hpp"

#include <Message_ProgressScope.hxx>
#include <algorithm>
#include <stdexcept>
#include <utility>

namespace application::tasks
{
    // Forwards OCCT progress to its context, and the context's cancel flag to OCCT.
    class TaskContext::Indicator final : public Message_ProgressIndicator
    {
      public:
        explicit Indicator(TaskContext& context)
            : _context(context)
        { }

        bool UserBreak() override
        {
            return _context.cancelled();
        }

        void Show(const Message_ProgressScope&, const bool) override
        {
            _context.setProgress(GetPosition());
        }

      private:
        TaskContext& _context;
    };

    TaskContext::TaskContext()  = default;
    TaskContext::~TaskContext() = default;

    void TaskContext::setOnChange(std::function<void()> callback)
    {
        _onChange = std::move(callback);
    }

    void TaskContext::setProgress(double fraction) noexcept
    {
        {
            const std::lock_guard lock(_mutex);
            _progress = std::clamp(fraction, 0.0, 1.0);
        }
        _changed();
    }

    void TaskContext::setIndeterminate(std::string status)
    {
        {
            const std::lock_guard lock(_mutex);
            _progress.reset();
            _status = std::move(status);
        }
        _changed();
    }

    void TaskContext::_changed() noexcept
    {
        if (!_onChange)
            return;
        try
        {
            _onChange();
        }
        catch (...)
        {
            // A lost progress notification only delays the bar; never let it
            // unwind through the OCCT algorithm that reported progress.
        }
    }

    bool TaskContext::cancelled() const noexcept
    {
        return _cancelled;
    }

    void TaskContext::cancel() noexcept
    {
        _cancelled = true;
    }

    Message_ProgressRange TaskContext::progressRange()
    {
        if (!_indicator.IsNull())
            throw std::logic_error("TaskContext::progressRange() can only be called once");
        _indicator = new Indicator(*this);
        return _indicator->Start();
    }

    std::optional<double> TaskContext::progress() const
    {
        const std::lock_guard lock(_mutex);
        return _progress;
    }

    std::string TaskContext::status() const
    {
        const std::lock_guard lock(_mutex);
        return _status;
    }
} // namespace application::tasks
