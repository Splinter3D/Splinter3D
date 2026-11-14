#include "settings_registry.hpp"

#include <algorithm>
#include <fstream>
#include <system_error>
#include <utility>

namespace platform::settings
{
    namespace
    {
        bool accepts(const Setting& setting, const std::string& value)
        {
            switch (setting.kind)
            {
                case Kind::Toggle:
                    return value == "0" || value == "1";
                case Kind::Choice:
                    return std::any_of(setting.choices.begin(), setting.choices.end(),
                                       [&](const Choice& choice) { return choice.value == value; });
                case Kind::KeyBind:
                    return value.find('\n') == std::string::npos;
                default:
                    return false;
            }
        }
    } // namespace

    Subscription::Subscription(SettingsRegistry* registry, std::size_t id)
        : registry_(registry)
        , id_(id)
    { }

    Subscription::~Subscription()
    {
        reset();
    }

    Subscription::Subscription(Subscription&& other) noexcept
        : registry_(std::exchange(other.registry_, nullptr))
        , id_(other.id_)
    { }

    Subscription& Subscription::operator=(Subscription&& other) noexcept
    {
        if (this != &other)
        {
            reset();
            registry_ = std::exchange(other.registry_, nullptr);
            id_       = other.id_;
        }
        return *this;
    }

    void Subscription::reset()
    {
        if (registry_ != nullptr)
            std::exchange(registry_, nullptr)->unsubscribe(id_);
    }

    SettingsRegistry& SettingsRegistry::getInstance()
    {
        static SettingsRegistry instance;
        return instance;
    }

    void SettingsRegistry::add(Setting setting)
    {
        const auto found = std::find_if(settings_.begin(), settings_.end(),
                                        [&](const Setting& s) { return s.key == setting.key; });
        if (found != settings_.end())
            *found = std::move(setting);
        else
            settings_.push_back(std::move(setting));
    }

    Subscription SettingsRegistry::add(Setting setting, Callback onChange)
    {
        std::string key = setting.key;
        add(std::move(setting));
        return subscribe(key, std::move(onChange));
    }

    Subscription SettingsRegistry::subscribe(const std::string& key, Callback onChange)
    {
        const std::size_t id = next_id_++;
        subscribers_.emplace(id, std::make_pair(key, std::move(onChange)));
        return Subscription(this, id);
    }

    void SettingsRegistry::unsubscribe(std::size_t id)
    {
        subscribers_.erase(id);
    }

    const std::vector<Setting>& SettingsRegistry::settings() const noexcept
    {
        return settings_;
    }

    const Setting* SettingsRegistry::find(const std::string& key) const
    {
        const auto found = std::find_if(settings_.begin(), settings_.end(), [&](const Setting& s) { return s.key == key; });
        return found != settings_.end() ? &*found : nullptr;
    }

    std::string SettingsRegistry::get(const std::string& key) const
    {
        const Setting* setting = find(key);
        if (const auto value = values_.find(key); value != values_.end())
            if (setting == nullptr || accepts(*setting, value->second))
                return value->second;
        return setting != nullptr ? setting->defaultValue : std::string();
    }

    bool SettingsRegistry::isOn(const std::string& key) const
    {
        return get(key) == "1";
    }

    void SettingsRegistry::set(const std::string& key, const std::string& value)
    {
        if (const Setting* setting = find(key); setting != nullptr && !accepts(*setting, value))
            return;
        const std::string previous = get(key);
        values_[key]               = value;
        save();
        if (previous == value)
            return;

        // A callback may (un)subscribe: walk the ids as they were, skipping gone ones.
        std::vector<std::size_t> ids;
        for (const auto& [id, subscriber] : subscribers_)
            if (subscriber.first == key)
                ids.push_back(id);
        for (const std::size_t id : ids)
            if (const auto found = subscribers_.find(id); found != subscribers_.end())
            {
                const Callback callback = found->second.second; // may unsubscribe itself
                callback(value);
            }
    }

    void SettingsRegistry::reset(const std::string& key)
    {
        const Setting* setting = find(key);
        set(key, setting != nullptr ? setting->defaultValue : std::string());
    }

    void SettingsRegistry::load(const std::filesystem::path& path)
    {
        path_ = path;
        std::ifstream file(path);
        // Same format as PreferenceManager: "key value", the value being the rest of the line.
        std::string line;
        while (std::getline(file, line))
        {
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            const auto separator = line.find(' ');
            if (separator == std::string::npos || separator == 0)
                continue;
            values_[line.substr(0, separator)] = line.substr(separator + 1);
        }
    }

    void SettingsRegistry::save() const
    {
        if (path_.empty())
            return;
        std::error_code ec;
        std::filesystem::create_directories(path_.parent_path(), ec);
        std::ofstream file(path_, std::ios::trunc);
        for (const auto& [key, value] : values_)
            file << key << ' ' << value << '\n';
    }

#if defined(SPLINTER3D_TESTING)
    void SettingsRegistry::clear()
    {
        settings_.clear();
        values_.clear();
        subscribers_.clear(); // next_id_ goes on: a stale Subscription must not hit a new subscriber
        path_.clear();
    }
#endif
} // namespace platform::settings
