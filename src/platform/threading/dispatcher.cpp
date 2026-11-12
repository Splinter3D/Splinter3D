#include "dispatcher.hpp"

#include "core/utils/version.hpp"
#include "platform/logger/logger.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <optional>
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

        // $SPLINTER3D_PROFILES, if set.
        std::optional<std::filesystem::path> overriddenProfiles()
        {
            const char* override = std::getenv("SPLINTER3D_PROFILES");
            if (override == nullptr || *override == '\0')
                return std::nullopt;
            // UTF-8, as every path string here.
            const std::string utf8 = override;
            return std::filesystem::path(std::u8string(utf8.begin(), utf8.end()));
        }
    } // namespace

    Dispatcher::Dispatcher()
        : _pool(poolThreads())
    { }

    std::filesystem::path Dispatcher::profilesPath(const std::filesystem::path& directory)
    {
        if (auto overridden = overriddenProfiles())
            return *overridden;
        return directory / kProfilesFile;
    }

    std::filesystem::path Dispatcher::locateProfiles(const std::filesystem::path& userDir, const std::filesystem::path& dataRoot)
    {
        const std::filesystem::path legacy = dataRoot / "assets" / kProfilesFile;
        if (auto overridden = overriddenProfiles())
            return *overridden;
        if (userDir.empty())
            return legacy;

        const std::filesystem::path target = userDir / kProfilesFile;
        std::error_code             ec;
        if (std::filesystem::exists(target, ec) || !std::filesystem::is_regular_file(legacy, ec))
            return target; // already there, or nothing to move
        std::filesystem::create_directories(target.parent_path(), ec);
        if (!ec && std::filesystem::copy_file(legacy, target, ec) && !ec)
        {
            std::filesystem::remove(legacy, ec); // may well fail on a read-only install: harmless
            logger::clog("[dispatcher] moved scalability profiles to ", target.string());
        }
        return target;
    }

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

    void Dispatcher::post(std::function<void()> fn)
    {
        _pool.post([fn = std::move(fn)] {
            try
            {
                fn();
            }
            catch (const std::exception& error)
            {
                platform::logger::cerr("[dispatcher] posted work threw: ", error.what());
            }
            catch (...)
            {
                platform::logger::cerr("[dispatcher] posted work threw a non-standard exception");
            }
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
        // Counted on the profile's recorded runs, which are saved: short
        // sessions (a batch script, a few imports) explore too.
        if (entry.profile.runs() % kExploreEvery == kExploreEvery - 1 && preferred && *preferred <= cap && cap > 1)
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
            // Too short a run says more about overhead than about scaling.
            if (succeeded && report && report->time && *report->time >= _minLearned)
                _operation(ticket.operation).profile.record(ticket.budget, *report, ticket.contended);
        }
        catch (...)
        {
            // Out of memory while recording: the run still happened, only its sample is lost.
        }
        if (!ticket.nested)
            _capacityFreed();
    }

    std::uint64_t Dispatcher::onCapacityFreed(CapacityListener listener)
    {
        const std::lock_guard lock(_listenersMutex);
        const std::uint64_t   token = _nextToken++;
        _listeners.emplace(token, std::move(listener));
        return token;
    }

    void Dispatcher::unsubscribe(std::uint64_t token)
    {
        const std::lock_guard lock(_listenersMutex);
        _listeners.erase(token);
    }

    void Dispatcher::_capacityFreed() noexcept
    {
        const std::lock_guard lock(_listenersMutex);
        for (const auto& [token, listener] : _listeners)
        {
            try
            {
                listener();
            }
            catch (...)
            {
                // A listener's failure must not break the run that freed the workers.
            }
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
            // Another release may scale differently (new algorithms, other
            // operations): learn again rather than trust its numbers.
            else if (const auto app = root.find("appVersion");
                     app == root.end() || !app->is_string() || app->get<std::string>() != core::utils::kVersion)
                platform::logger::clog("[dispatcher] discarding profiles in ", path, ": learned by another Splinter3D version (",
                                       app != root.end() && app->is_string() ? app->get<std::string>() : std::string("unknown"),
                                       ", now ", core::utils::kVersion, ")");
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
            {"appVersion", core::utils::kVersion},
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

    void Dispatcher::setMinLearnedDuration(benchmark::Duration duration)
    {
        const std::lock_guard lock(_mutex);
        _minLearned = duration;
    }

    void Dispatcher::setProfile(std::string_view operation, ScalabilityProfile replacement)
    {
        const std::lock_guard lock(_mutex);
        _operation(operation).profile = std::move(replacement);
    }
#endif
} // namespace platform::threading
