#pragma once

#include <Objects3D/OTriangle.hpp>
#include <Objects3D/OVec3.hpp>
#include <Renderer/Color.hpp>
#include <Renderer/RTriangle.hpp>
#include <Renderer/RVec3.hpp>
#include <vector>

namespace objects3D { struct OMesh; } // forward declaration

namespace renderer
{
    struct RMesh
    {
        std::vector<RTriangle> triangles;

        RMesh() : _parentMesh(nullptr), _color(255, 255, 255, 255)
        { }

        RMesh(const std::vector<RTriangle>& tris, Color color) : triangles(tris), _color(color)
        { }

        /**
         * @brief Creates an RMesh from an OMesh. This method converts the triangles from the OMesh format to the RMesh format and assigns a uniform color to all triangles. It also sets up a back-reference to the OMesh for synchronization.
         * @param mesh A pointer to the OMesh to be converted into an RMesh.
         * @param color The color to be applied to all triangles in the resulting RMesh.
         * @return An RMesh containing the triangles from the OMesh, all colored uniformly.
         */
        static RMesh fromOMesh(objects3D::OMesh* mesh, Color color);

        /**
         * @brief Updates this RMesh's triangles to match the current state of the attached OMesh.
         */
        void updateFromOMesh();

        /**
         * @brief Set the color of teh mesh.
         */
        void setMeshColor(Color color);

        private:
            objects3D::OMesh* _parentMesh;
            Color             _color;

            void attachToOMesh(objects3D::OMesh* mesh);
            void detachFromOMesh();
            void loadTrianglesFromOMesh(objects3D::OMesh* mesh);
    };
} // namespace renderer
