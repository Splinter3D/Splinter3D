#include "dispatcher.hpp"

#include "platform/logger/logger.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <thread>

namespace platform::threading
{
    namespace
    {
        constexpr int kProfilesVersion = 1;

        // One core stays with the UI thread.
        std::size_t poolThreads() noexcept
        {
            const unsigned cores = std::thread::hardware_concurrency();
            return cores > 1 ? cores - 1 : 1;
        }
    } // namespace

    Dispatcher::Dispatcher()
        : _pool(poolThreads())
    { }

    std::size_t Dispatcher::nproc() const noexcept
    {
        const auto& context = ExecutionContext::current();
        return context ? context->budget : 1;
    }

    std::size_t Dispatcher::total() const noexcept
    {
        return _pool.size() + 1;
    }

    void Dispatcher::submit(std::string operation, std::function<void()> fn, std::function<void()> then)
    {
        _pool.post([this, operation = std::move(operation), fn = std::move(fn), then = std::move(then)] {
            try
            {
                run(operation, fn);
            }
            catch (const std::exception& error)
            {
                platform::logger::cerr("[dispatcher] '", operation, "' threw: ", error.what());
            }
            catch (...)
            {
                platform::logger::cerr("[dispatcher] '", operation, "' threw a non-standard exception");
            }
            if (then)
                then();
        });
    }

    std::size_t Dispatcher::freeWorkers() const noexcept
    {
        const std::lock_guard lock(_mutex);
        return total() - std::min(_allocated, total());
    }

    void Dispatcher::parallelFor(std::size_t first, std::size_t last, const std::function<void(std::size_t)>& body)
    {
        _pool.parallelFor(first, last, nproc(), body);
    }

    std::optional<ScalabilityProfile> Dispatcher::profile(std::string_view operation) const
    {
        const std::lock_guard lock(_mutex);
        const auto            found = _operations.find(operation);
        if (found == _operations.end() || found->second.profile.runs() == 0)
            return std::nullopt;
        return found->second.profile;
    }

    Dispatcher::Operation& Dispatcher::_operation(std::string_view operation)
    {
        const auto found = _operations.find(operation);
        if (found != _operations.end())
            return found->second;
        return _operations.emplace(std::string(operation), Operation{}).first->second;
    }

    Dispatcher::Ticket Dispatcher::_acquire(std::string_view operation)
    {
        const std::lock_guard lock(_mutex);
        Operation&            entry  = _operation(operation);
        const auto&           parent = ExecutionContext::current();

        // A nested run shares its parent's workers; a top-level one takes from what is free.
        const std::size_t cap       = parent ? parent->budget : std::max<std::size_t>(1, total() - std::min(_allocated, total()));
        const auto        preferred = entry.profile.preferredWorkers(total());

        std::size_t budget = 0;
        if (preferred)
            budget = *preferred;
        else
            budget = parent ? cap : cap / (_active + 1);

        // Explore only when the cap leaves room for it; otherwise the attempt
        // would be clamped away and still use up a direction.
        if (++entry.dispatches % kExploreEvery == 0 && preferred && *preferred <= cap && cap > 1)
        {
            bool up = entry.exploreUp;
            if (*preferred >= cap)
                up = false;
            else if (*preferred <= 1)
                up = true;
            budget          = up ? *preferred + 1 : *preferred - 1;
            entry.exploreUp = !entry.exploreUp;
        }
        budget = std::clamp<std::size_t>(budget, 1, cap);

        Ticket ticket{.operation = std::string(operation), .budget = budget, .nested = parent.has_value()};
        if (!ticket.nested)
        {
            _allocated += budget;
            ++_active;
        }
        ticket.contended = _active > 1;
        return ticket;
    }

    void Dispatcher::_release(Ticket& ticket, const std::optional<benchmark::Report>& report, bool succeeded) noexcept
    {
        try
        {
            const std::lock_guard lock(_mutex);
            ticket.contended = ticket.contended || _active > 1;
            if (!ticket.nested)
            {
                _allocated -= ticket.budget;
                --_active;
            }
            if (succeeded && report)
                _operation(ticket.operation).profile.record(ticket.budget, *report, ticket.contended);
        }
        catch (...)
        {
            // Out of memory while recording: the run still happened, only its sample is lost.
        }
    }

    void Dispatcher::load(const std::filesystem::path& path)
    {
        std::map<std::string, ScalabilityProfile, std::less<>> loaded;

        std::ifstream file(path);
        if (file)
        {
            const nlohmann::json root = nlohmann::json::parse(file, nullptr, false);
            if (root.is_discarded() || !root.is_object())
                platform::logger::cerr("[dispatcher] ignoring profiles in ", path, ": not a JSON object");
            else if (const auto version = root.find("version");
                     version == root.end() || !version->is_number_integer() || *version != kProfilesVersion)
                platform::logger::cerr("[dispatcher] ignoring profiles in ", path, ": unsupported version");
            else if (const auto operations = root.find("operations");
                     operations != root.end() && operations->is_object())
            {
                for (const auto& [name, node] : operations->items())
                {
                    if (auto parsed = ScalabilityProfile::fromJson(node, total()))
                        loaded.emplace(name, std::move(*parsed));
                    else
                        platform::logger::cerr("[dispatcher] ignoring malformed profile '", name, "' in ", path);
                }
            }
        }

        const std::lock_guard lock(_mutex);
        _path = path;
        for (auto& [name, entry] : _operations)
            entry.profile = ScalabilityProfile{};
        for (auto& [name, parsed] : loaded)
            _operation(name).profile = std::move(parsed);
    }

    void Dispatcher::save() const
    {
        std::filesystem::path path;
        {
            const std::lock_guard lock(_mutex);
            path = _path;
        }
        if (!path.empty())
            save(path);
    }

    void Dispatcher::save(const std::filesystem::path& path) const
    {
        nlohmann::json operations = nlohmann::json::object();
        {
            const std::lock_guard lock(_mutex);
            for (const auto& [name, entry] : _operations)
                if (entry.profile.runs() > 0)
                    operations[name] = entry.profile.toJson();
        }

        if (operations.empty())
            return;

        const nlohmann::json root = {
            {"version", kProfilesVersion},
            {"hardwareConcurrency", std::thread::hardware_concurrency()},
            {"operations", std::move(operations)},
        };

        std::error_code ec;
        if (path.has_parent_path())
            std::filesystem::create_directories(path.parent_path(), ec);

        std::filesystem::path temporary = path;
        temporary += ".tmp";
        {
            std::ofstream file(temporary, std::ios::trunc);
            file << root.dump(2) << '\n';
            file.close();
            if (!file)
            {
                platform::logger::cerr("[dispatcher] could not save profiles to ", path);
                std::filesystem::remove(temporary, ec);
                return;
            }
        }
        std::filesystem::rename(temporary, path, ec);
        if (ec)
        {
            platform::logger::cerr("[dispatcher] could not save profiles to ", path, ": ", ec.message());
            std::filesystem::remove(temporary, ec);
        }
    }

#if defined(SPLINTER3D_TESTING)
    void Dispatcher::clear()
    {
        const std::lock_guard lock(_mutex);
        _operations.clear();
        _path.clear();
    }

    void Dispatcher::setProfile(std::string_view operation, ScalabilityProfile replacement)
    {
        const std::lock_guard lock(_mutex);
        _operation(operation).profile = std::move(replacement);
    }
#endif
} // namespace platform::threading
