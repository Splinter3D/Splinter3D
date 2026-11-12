#include "task_manager.hpp"

#include "platform/threading/dispatcher.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <exception>
#include <utility>

namespace application::tasks
{
    namespace
    {
        bool overlaps(const std::vector<service::ModelId>& lhs, const std::vector<service::ModelId>& rhs)
        {
            return std::any_of(lhs.begin(), lhs.end(), [&](service::ModelId id) {
                return std::find(rhs.begin(), rhs.end(), id) != rhs.end();
            });
        }

        std::int64_t nowMs() noexcept
        {
            return std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::steady_clock::now().time_since_epoch())
                .count();
        }
    } // namespace

    TaskManager::TaskManager(Poster poster)
        : _poster(std::move(poster))
    {
        if (!_poster)
            return; // synchronous: nothing ever waits for workers
        // Called on the releasing (worker) thread: only post a reschedule.
        _capacityToken = platform::threading::Dispatcher::getInstance().onCapacityFreed(
            [this, shared = _shared, poster = _poster] {
                if (shared->rescheduleQueued.exchange(true))
                    return;
                poster([this, shared] {
                    shared->rescheduleQueued = false;
                    if (!shared->stopped)
                        _schedule();
                });
            });
    }

    TaskManager::~TaskManager()
    {
        cancelAllAndWait();
    }

    TaskId TaskManager::submit(Task task)
    {
        if (_shared->stopped)
            return 0; // after cancelAllAndWait(): refused
        const TaskId id = _nextId++;

        Entry entry;
        entry.id   = id;
        entry.task = std::move(task);
        _entries.push_back(std::move(entry));

        if (!_poster)
        {
            _runInline(_entries.back());
            return id;
        }

        // An instant edit on free models: no card, applied right away.
        if (!_entries.back().task.work && _blocker(_entries.back()) == nullptr)
        {
            const Task edit  = _entries.back().task; // apply() may submit more tasks
            auto       error = edit.prepare ? _guarded(edit.prepare) : std::nullopt;
            if (!error && edit.apply)
                error = _guarded([&] {
                    edit.apply();
                    return std::optional<std::string>();
                });
            if (error)
                _fail(*_find(id), std::move(*error)); // a card, so the user sees why
            else
                std::erase_if(_entries, [id](const Entry& other) { return other.id == id; });
            return id;
        }

        _notify(_entries.back());
        _schedule();
        return id;
    }

    void TaskManager::cancel(TaskId id)
    {
        Entry* entry = _find(id);
        if (entry == nullptr)
            return;
        if (entry->state == TaskState::Queued)
        {
            _discard(*entry);
            _remove(id);
            _schedule();
        }
        else if (entry->state == TaskState::Running)
        {
            entry->state = TaskState::Cancelling;
            entry->context->cancel();
            _notify(*entry);
        }
    }

    void TaskManager::dismiss(TaskId id)
    {
        const Entry* entry = _find(id);
        if (entry != nullptr && entry->state == TaskState::Failed)
            _remove(id);
    }

    void TaskManager::cancelAllAndWait()
    {
        // First: once this returns, the listener neither runs nor posts again.
        if (_capacityToken != 0)
        {
            platform::threading::Dispatcher::getInstance().unsubscribe(_capacityToken);
            _capacityToken = 0;
        }
        for (auto& entry : _entries)
            if (entry.context)
                entry.context->cancel();

        {
            std::unique_lock lock(_shared->mutex);
            _shared->settled.wait(lock, [this] { return _shared->inFlight == 0; });
            _shared->stopped = true;
        }
        for (auto& entry : _entries)
            _discard(entry); // none of them will apply now
        _entries.clear();
    }

    std::size_t TaskManager::pending() const
    {
        return static_cast<std::size_t>(std::count_if(_entries.begin(), _entries.end(), [](const Entry& entry) {
            return entry.state != TaskState::Failed;
        }));
    }

    bool TaskManager::idle() const
    {
        return pending() == 0;
    }

    bool TaskManager::wouldWait(service::ModelId id) const
    {
        const auto mentions = [id](const std::vector<service::ModelId>& ids) {
            return std::find(ids.begin(), ids.end(), id) != ids.end();
        };
        return std::any_of(_entries.begin(), _entries.end(), [&](const Entry& entry) {
            return (entry.state != TaskState::Failed && mentions(entry.task.writes)) ||
                   (entry.state == TaskState::Queued && mentions(entry.task.reads));
        });
    }

    bool TaskManager::hasPending(std::string_view operation, service::ModelId id) const
    {
        return std::any_of(_entries.begin(), _entries.end(), [&](const Entry& entry) {
            // A cancelling task won't produce anything: it doesn't count.
            return (entry.state == TaskState::Queued || entry.state == TaskState::Running) &&
                   entry.task.operation == operation &&
                   std::find(entry.task.writes.begin(), entry.task.writes.end(), id) != entry.task.writes.end();
        });
    }

    std::optional<TaskSnapshot> TaskManager::snapshot(TaskId id) const
    {
        const Entry* entry = _find(id);
        if (entry == nullptr)
            return std::nullopt;
        return _snapshot(*entry);
    }

    std::vector<TaskSnapshot> TaskManager::snapshots() const
    {
        std::vector<TaskSnapshot> result;
        result.reserve(_entries.size());
        for (const auto& entry : _entries)
            result.push_back(_snapshot(entry));
        return result;
    }

    void TaskManager::setListener(Listener listener)
    {
        _listener = std::move(listener);
    }

    TaskManager::Entry* TaskManager::_find(TaskId id)
    {
        const auto found =
            std::find_if(_entries.begin(), _entries.end(), [id](const Entry& entry) { return entry.id == id; });
        return found == _entries.end() ? nullptr : &*found;
    }

    const TaskManager::Entry* TaskManager::_find(TaskId id) const
    {
        const auto found =
            std::find_if(_entries.begin(), _entries.end(), [id](const Entry& entry) { return entry.id == id; });
        return found == _entries.end() ? nullptr : &*found;
    }

    // The earliest unfinished task before `entry` that writes a model `entry` reads or writes.
    const TaskManager::Entry* TaskManager::_blocker(const Entry& entry) const
    {
        for (const auto& earlier : _entries)
        {
            if (earlier.id == entry.id)
                break;
            if (earlier.state == TaskState::Failed)
                continue;
            if (overlaps(earlier.task.writes, entry.task.writes) || overlaps(earlier.task.writes, entry.task.reads))
                return &earlier;
            // A read takes its snapshot when it starts: until then, later
            // writes to what it reads must wait, or it would see them.
            if (earlier.state == TaskState::Queued && overlaps(earlier.task.reads, entry.task.writes))
                return &earlier;
        }
        return nullptr;
    }

    TaskSnapshot TaskManager::_snapshot(const Entry& entry) const
    {
        TaskSnapshot snapshot;
        snapshot.id    = entry.id;
        snapshot.name  = entry.task.name;
        snapshot.state = entry.state;
        switch (entry.state)
        {
            case TaskState::Queued:
                snapshot.status = entry.waitingFor.empty() ? "Queued" : "Queued · " + entry.waitingFor;
                break;
            case TaskState::Running:
                if (entry.context)
                {
                    snapshot.progress = entry.context->progress();
                    snapshot.status   = entry.context->status();
                }
                break;
            case TaskState::Cancelling:
                snapshot.status = "Cancelling…";
                break;
            case TaskState::Failed:
                snapshot.error = entry.error;
                break;
            default:
                break;
        }
        return snapshot;
    }

    void TaskManager::_schedule()
    {
        if (_scheduling || _shared->stopped)
            return;
        _scheduling = true;

        // Reset even if a step throws, or no task would ever start again.
        struct Reset
        {
            bool& flag;

            ~Reset()
            {
                flag = false;
            }
        } const reset{_scheduling};

        // Starting a task can change _entries (an instant edit is removed), so
        // rescan from the top after each start.
        for (bool started = true; started;)
        {
            started = false;
            for (auto& entry : _entries)
            {
                if (entry.state != TaskState::Queued)
                    continue;

                std::string waitingFor;
                if (const Entry* blocker = _blocker(entry))
                    waitingFor = "waiting for \"" + blocker->task.name + "\"";
                // Until a launched task holds its workers, freeWorkers() still
                // counts them: wait for it (it reschedules once it has them).
                else if (entry.task.work && (_shared->launching.load() > 0 ||
                                             platform::threading::Dispatcher::getInstance().freeWorkers() == 0))
                    waitingFor = "waiting for workers";

                if (!waitingFor.empty())
                {
                    if (waitingFor != entry.waitingFor)
                    {
                        entry.waitingFor = std::move(waitingFor);
                        _notify(entry);
                    }
                    continue;
                }

                _start(entry);
                started = true;
                break;
            }
        }
    }

    void TaskManager::_start(Entry& entry)
    {
        const TaskId id      = entry.id;
        const auto   prepare = entry.task.prepare; // steps may submit more tasks and move entries
        const auto   apply   = entry.task.apply;
        const bool   hasWork = static_cast<bool>(entry.task.work);
        entry.state          = TaskState::Running;
        entry.waitingFor.clear();

        if (prepare)
        {
            if (auto error = _guarded(prepare))
            {
                _fail(*_find(id), std::move(*error));
                return;
            }
        }

        if (!hasWork)
        {
            if (apply)
            {
                if (auto error = _guarded([&] {
                        apply();
                        return std::optional<std::string>();
                    }))
                {
                    _fail(*_find(id), std::move(*error));
                    return;
                }
            }
            _remove(id);
            return;
        }

        Entry& started = *_find(id);

        started.context = std::make_shared<TaskContext>();
        auto shared     = _shared;
        auto poster     = _poster;

        // Progress: at most one notification per kProgressInterval.
        auto lastPost = std::make_shared<std::atomic<std::int64_t>>(0);
        started.context->setOnChange([this, shared, poster, id, lastPost] {
            const std::int64_t now = nowMs();
            if (now - lastPost->load() < kProgressInterval.count())
                return;
            lastPost->store(now);
            poster([this, shared, id] {
                if (!shared->stopped)
                    _progressed(id);
            });
        });

        auto error   = std::make_shared<std::optional<std::string>>();
        auto began   = std::make_shared<std::atomic<bool>>(false);
        auto work    = started.task.work;
        auto context = started.context;
        _notify(started); // before the counters: nothing can leak them from here
        {
            const std::lock_guard lock(shared->mutex);
            ++shared->inFlight;
        }
        ++shared->launching;

        try
        {
            platform::threading::Dispatcher::getInstance().submit(
                started.task.operation,
                [this, shared, poster, work, context, error, began] {
                    *began = true;
                    --shared->launching; // the run holds its workers now: let the next one launch
                    try
                    {
                        poster([this, shared] {
                            if (!shared->stopped)
                                _schedule();
                        });
                    }
                    catch (...)
                    {
                        // Out of memory: the next finish reschedules.
                    }
                    try
                    {
                        work(*context);
                    }
                    catch (const std::exception& exception)
                    {
                        *error = exception.what();
                    }
                    catch (...)
                    {
                        *error = "unknown error";
                    }
                },
                // After the budget is released, so _finish() sees the freed workers.
                [this, shared, poster, id, error, began] {
                    if (!*began) // run() failed before calling us
                        --shared->launching;
                    try
                    {
                        // Posted before inFlight drops: cancelAllAndWait() can't
                        // return (and the poster's owner go away) before this.
                        poster([this, shared, id, error] {
                            if (!shared->stopped)
                                _finish(id, *error);
                        });
                    }
                    catch (...)
                    {
                        // Out of memory: the card stays until cancelAllAndWait().
                    }
                    {
                        const std::lock_guard lock(shared->mutex);
                        --shared->inFlight;
                    }
                    shared->settled.notify_all();
                });
        }
        catch (const std::exception& exception)
        {
            {
                const std::lock_guard lock(shared->mutex);
                --shared->inFlight;
            }
            --shared->launching;
            _fail(*_find(id), exception.what());
        }
    }

    void TaskManager::_finish(TaskId id, std::optional<std::string> error)
    {
        Entry* entry = _find(id);
        if (entry != nullptr)
        {
            if (entry->state == TaskState::Cancelling)
            {
                _discard(*entry);
                _remove(id);
            }
            else if (error)
                _fail(*entry, std::move(*error));
            else
            {
                const auto apply = entry->task.apply;
                try
                {
                    if (apply)
                        apply();
                    _remove(id);
                }
                catch (const std::exception& exception)
                {
                    if (Entry* failed = _find(id))
                        _fail(*failed, exception.what());
                }
            }
        }
        _schedule();
    }

    void TaskManager::_progressed(TaskId id)
    {
        const Entry* entry = _find(id);
        if (entry != nullptr && entry->state == TaskState::Running)
            _notify(*entry);
    }

    void TaskManager::_fail(Entry& entry, std::string error)
    {
        _discard(entry);
        entry.state = TaskState::Failed;
        entry.error = std::move(error);
        entry.waitingFor.clear();
        _notify(entry);
    }

    void TaskManager::_remove(TaskId id)
    {
        const auto found =
            std::find_if(_entries.begin(), _entries.end(), [id](const Entry& entry) { return entry.id == id; });
        if (found == _entries.end())
            return;
        const TaskSnapshot last = _snapshot(*found);
        _entries.erase(found);
        if (_poster && _listener)
            _tell(last, true);
    }

    void TaskManager::_notify(const Entry& entry, bool removed)
    {
        if (_poster && _listener)
            _tell(_snapshot(entry), removed);
    }

    void TaskManager::_tell(const TaskSnapshot& snapshot, bool removed) noexcept
    {
        try
        {
            _listener(snapshot, removed);
        }
        catch (...)
        {
            // A listener failing to show a card must not break the scheduling.
        }
    }

    void TaskManager::_discard(Entry& entry) noexcept
    {
        const auto discard = std::exchange(entry.task.discard, {}); // once
        if (!discard)
            return;
        try
        {
            discard();
        }
        catch (...)
        {
            // Best effort: a leftover (e.g. a temporary file) is all that is lost.
        }
    }

    std::optional<std::string> TaskManager::_guarded(const std::function<std::optional<std::string>()>& step)
    {
        try
        {
            return step();
        }
        catch (const std::exception& exception)
        {
            return std::string(exception.what());
        }
        catch (...)
        {
            return std::string("unknown error");
        }
    }

    void TaskManager::_runInline(Entry& entry)
    {
        const TaskId id   = entry.id;
        const Task   task = entry.task; // callbacks may submit more tasks and move entries

        if (task.prepare)
        {
            if (auto error = task.prepare())
            {
                _fail(*_find(id), std::move(*error));
                return;
            }
        }
        try
        {
            if (task.work)
            {
                TaskContext context;
                platform::threading::Dispatcher::getInstance().run(task.operation, [&] { task.work(context); });
            }
            if (task.apply)
                task.apply();
        }
        catch (const std::exception& exception)
        {
            _fail(*_find(id), exception.what());
            return;
        }
        catch (...)
        {
            _fail(*_find(id), "unknown error");
            return;
        }
        _remove(id);
    }
} // namespace application::tasks
