#pragma once

#include "geometry/occt/shape.hpp"
#include "geometry/occt/transform.hpp"

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace application::service
{
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
            std::string               name;
            geometry::occt::Shape     shape;
            geometry::occt::Transform transform;
        };

        /**
         * @brief Imports an STL file and appends it to the scene.
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
         * @return Index of the new model.
         */
        std::size_t add(const std::string& baseName, geometry::occt::Shape shape);

        /// Removes every model.
        void clear() noexcept;

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

        /**
         * @brief Writes the scene to an STL file.
         *
         * @return false if the scene is empty or the file could not be written.
         */
        [[nodiscard]] bool saveSTL(const std::filesystem::path& path) const;

      private:
        [[nodiscard]] bool        isValidIndex(int index) const noexcept;
        [[nodiscard]] std::string uniqueName(const std::string& base) const;

        std::vector<Model> models_;
    };
} // namespace application::service
