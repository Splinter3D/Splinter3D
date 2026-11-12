#pragma once

#include <cstddef>
#include <filesystem>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace platform::settings
{
    /** @brief How a setting is edited (see ui::panels::SettingsPanel). */
    enum class Kind
    {
        Toggle,  // "1" or "0"
        Choice,  // one of Setting::choices' values
        KeyBind, // a key combination, as wxAcceleratorEntry::ToRawString() writes it ("Ctrl+Shift+O")
    };

    struct Choice
    {
        std::string value;
        std::string label; // locale key
    };

    /**
     * @brief A user setting a class declares: what it is, how to show it, and
     * its default. Its current value lives in the SettingsRegistry.
     */
    struct Setting
    {
        std::string         key;     // also its key in the settings file, e.g. "tasks.auto_open"
        std::string         section; // locale key of the heading it is listed under
        std::string         label;   // locale key
        std::string         note;    // locale key of a hint shown under it (e.g. "applies after restart"); may be empty
        Kind                kind = Kind::Toggle;
        std::string         defaultValue;
        std::vector<Choice> choices; // Choice only
    };

    class SettingsRegistry;

    /** @brief Keeps a change callback registered; unregisters it when destroyed. */
    class Subscription
    {
      public:
        Subscription() = default;
        ~Subscription();
        Subscription(Subscription&& other) noexcept;
        Subscription& operator=(Subscription&& other) noexcept;
        Subscription(const Subscription&)            = delete;
        Subscription& operator=(const Subscription&) = delete;

        void reset();

      private:
        friend class SettingsRegistry;
        Subscription(SettingsRegistry* registry, std::size_t id);

        SettingsRegistry* registry_ = nullptr;
        std::size_t       id_       = 0;
    };

    /**
     * @brief The application's settings: classes declare theirs (add()), read
     * them (get()) and follow their changes (subscribe()); the settings panel
     * lists and edits them all.
     *
     * Values are strings, saved as "key value" lines to the file given to
     * load() on every change. A value read from the file is kept even for a
     * setting nobody declared (yet). UI thread only.
     */
    class SettingsRegistry
    {
      public:
        using Callback = std::function<void(const std::string& value)>;

        static SettingsRegistry& getInstance();

        /**
         * @brief Declares a setting, in the order the panel lists them.
         * Declaring a key again replaces its description and keeps its value.
         */
        void add(Setting setting);
        /** @brief add(), then subscribe() to it. */
        [[nodiscard]] Subscription add(Setting setting, Callback onChange);

        /** @brief Calls onChange with the new value each time key's value changes, until the subscription goes. */
        [[nodiscard]] Subscription subscribe(const std::string& key, Callback onChange);

        /** @brief Declared settings, in declaration order. */
        [[nodiscard]] const std::vector<Setting>& settings() const noexcept;
        [[nodiscard]] const Setting*              find(const std::string& key) const;

        /** @brief key's value, else its default, else "". */
        [[nodiscard]] std::string get(const std::string& key) const;
        /** @brief Whether key's value is "1" (a Toggle that is on). */
        [[nodiscard]] bool isOn(const std::string& key) const;

        /**
         * @brief Changes key's value, saves, and notifies its subscribers if it
         * changed. A value a declared setting can't take (a Toggle other than
         * "0"/"1", a Choice not listed) is ignored.
         */
        void set(const std::string& key, const std::string& value);
        /** @brief Back to the default. */
        void reset(const std::string& key);

        /** @brief Reads values from path (if it exists); every change is saved there from now on. */
        void load(const std::filesystem::path& path);

#if defined(SPLINTER3D_TESTING)
        /** @brief Test-only: forgets every setting, value, subscriber and the file. */
        void clear();
#endif

      private:
        friend class Subscription;

        void unsubscribe(std::size_t id);
        void save() const;

        std::vector<Setting>                                    settings_;
        std::map<std::string, std::string>                      values_;
        std::map<std::size_t, std::pair<std::string, Callback>> subscribers_;
        std::size_t                                             next_id_ = 1;
        std::filesystem::path                                   path_;
    };
} // namespace platform::settings
