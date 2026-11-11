#pragma once

#include "geometry/occt/shape.hpp"
#include "geometry/occt/transform.hpp"
#include "platform/benchmark/benchmark.hpp"

#include <Message_ProgressRange.hxx>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace application::service
{
    /**
     * @brief Identifies a model for as long as it exists, unlike its index,
     * which shifts when an earlier model is removed. Never reused; 0 is never
     * a valid id.
     */
    using ModelId = std::uint64_t;

    /**
     * @brief The set of models currently open, with their display names and
     * placements.
     *
     * This is the single source of truth for what is loaded and where each
     * model sits; ui/ only mirrors it on screen (see ModelViewerPanel). Models
     * are kept in load order and addressed by index; several operations also
     * accept kAllModels to target every model at once.
     */
    class SceneService
    {
      public:
        /// Target index meaning "every loaded model".
        static constexpr int kAllModels = -1;

        struct Model
        {
            ModelId                       id = 0;
            std::string                   name;
            geometry::occt::Shape         shape;
            geometry::occt::Transform     transform;
            platform::benchmark::Duration loadDuration{};
        };

        /**
         * @brief Imports an STL file and appends it to the scene.
         *
         * The import is timed: the duration is kept in Model::loadDuration and
         * logged (platform::logger::clog) as "[scene] imported ...".
         *
         * @return Index of the new model.
         * @throws std::invalid_argument / std::runtime_error from Shape::fromSTL.
         */
        std::size_t load(const std::filesystem::path& path);

        /**
         * @brief Appends an already-built shape to the scene.
         *
         * @param baseName Display name; made unique with a " (2)", " (3)", ...
         * suffix if another model already uses it.
         * @param id An id from reserveId(), or 0 to take a new one.
         * @param loadDuration How long importing it took, if it was imported.
         * @return Index of the new model.
         */
        std::size_t add(const std::string&            baseName,
                        geometry::occt::Shape         shape,
                        ModelId                       id           = 0,
                        platform::benchmark::Duration loadDuration = {});

        /**
         * @brief Takes the next model id without adding a model, so work that
         * will add one later (a background import) can be referred to now.
         */
        [[nodiscard]] ModelId reserveId();

        /**
         * @brief Ids reserved but not added yet (imports still running, or
         * that failed), so a task on "every model" can wait for them too.
         */
        [[nodiscard]] const std::vector<ModelId>& reservedIds() const noexcept;

        /** @brief Gives up a reserved id that will never be added (a failed or cancelled import). */
        void releaseId(ModelId id);

        /// Removes every model.
        void clear() noexcept;

        /**
         * @brief Removes one model; the following ones shift down by one index.
         *
         * @return false (and changes nothing) if the index is out of range.
         */
        bool remove(int index);

        /** @brief Removes the model with this id; false if there is none. */
        bool removeById(ModelId id);

        /** @brief The model with this id, or nullptr once it was removed. */
        [[nodiscard]] const Model* find(ModelId id) const noexcept;

        /** @brief Current index of the model with this id, if it still exists. */
        [[nodiscard]] std::optional<std::size_t> indexOf(ModelId id) const noexcept;

        /** @brief Ids of every model, in load order. */
        [[nodiscard]] std::vector<ModelId> ids() const;

        [[nodiscard]] bool        empty() const noexcept;
        [[nodiscard]] std::size_t size() const noexcept;

        [[nodiscard]] const std::vector<Model>& models() const noexcept;

        /// Display names of every model, in load order.
        [[nodiscard]] std::vector<std::string> names() const;

        /**
         * @brief Returns the placement of one model; identity for kAllModels
         * or an out-of-range index.
         */
        [[nodiscard]] geometry::occt::Transform transform(int targetIndex) const;

        /**
         * @brief Replaces the placement of one model, or of every model with
         * kAllModels.
         *
         * @return false (and changes nothing) if the scene is empty or the
         * index is out of range.
         */
        bool setTransform(int targetIndex, const geometry::occt::Transform& transform);

        /** @brief Replaces the placement of the model with this id; false if there is none. */
        bool setTransformById(ModelId id, const geometry::occt::Transform& transform);

        /**
         * @brief Writes the scene to an STL file.
         *
         * @return false if the scene is empty or the file could not be written.
         */
        [[nodiscard]] bool saveSTL(const std::filesystem::path& path) const;

      private:
        [[nodiscard]] bool        isValidIndex(int index) const noexcept;
        [[nodiscard]] std::string uniqueName(const std::string& base) const;

        std::vector<Model>   models_;
        ModelId              nextId_ = 1;
        std::vector<ModelId> reserved_;
    };

    /**
     * @brief Writes models (each placed at its transform) to one STL file.
     *
     * Meshes a copy of them if needed (the models' own shapes are never
     * changed), writes a uniquely named "<path>.<n>.tmp" and renames it over path, so
     * path is never left half-written: on failure or cancellation (a user
     * break on progress) the temporary file is deleted and path is untouched.
     *
     * @return false if models is empty, the write failed or it was cancelled.
     */
    /**
     * @brief The slow half of exportSTL(): meshes and writes a uniquely named
     * temporary file next to path, and returns it (empty on failure or
     * cancellation, leaving nothing behind). path itself is not touched.
     */
    [[nodiscard]] std::optional<std::filesystem::path> writeSTLTemporary(
        const std::vector<SceneService::Model>& models,
        const std::filesystem::path&            path,
        const Message_ProgressRange&            progress = Message_ProgressRange());

    /** @brief The quick half: renames temporary over path; on failure deletes it and returns false. */
    [[nodiscard]] bool commitSTLTemporary(const std::filesystem::path& temporary, const std::filesystem::path& path);

    [[nodiscard]] bool exportSTL(const std::vector<SceneService::Model>& models,
                                 const std::filesystem::path&            path,
                                 const Message_ProgressRange&            progress = Message_ProgressRange());
} // namespace application::service
