#pragma once

#include <Geometry/Mesh.hpp>
#include <Objects3D/ObjectObserver.hpp>
#include <Objects3D/Transform.hpp>
#include <Renderer/Color.hpp>
#include <algorithm>
#include <vector>

namespace objects3D
{
    struct Object3D
    {

#pragma region Object3D

        Object3D(geometry::Mesh* m);

        /**
         * Constructs a new Object3D by combining the meshes of the given objects.
         * All the objects' meshes will be merged into a single mesh for this Object3D.
         * @attention The original meshes of the input objects will NOT be modified.
         * @param objects The list of Object3D instances whose meshes will be combined to create this Object3D.
         * @param applyTransforms If true, the current transforms of the input objects will be applied to their meshes before merging.
         */
        Object3D(const std::vector<objects3D::Object3D>& objects, bool applyTransforms = true)
        {
            _mesh = new geometry::Mesh();
            for (const auto& obj : objects)
            {
                if (applyTransforms)
                {
                    geometry::Mesh* mesh = obj.getTransformedMesh();
                    _mesh->triangles.insert(_mesh->triangles.end(), mesh->triangles.begin(), mesh->triangles.end());
                }
                else
                {
                    geometry::Mesh* mesh = obj.getMesh();
                    _mesh->triangles.insert(_mesh->triangles.end(), mesh->triangles.begin(), mesh->triangles.end());
                }
            }
        }

        /**
         * Create a new Object3D by copying the mesh and transform of another Object3D. The observers are NOT copied.
         */
        Object3D(const Object3D& other);

        ~Object3D();

        /**
         * Load the mesh from an STL file and create an Object3D.
         */
        static Object3D fromSTL(const std::string& stlFile);

#pragma endregion
#pragma region Observers

        /**
         * Attaches an observer to this object. The observer will be notified when notifyTransform() or notifyAppearanceChanged() is called.
         */
        void attach(ObjectObserver* obs);

        /**
         * Detaches an observer from this object. The observer will no longer receive notifications.
         */
        void detach(ObjectObserver* obs);

        /**
         * Notify all the observers that the transform has changed.
         */
        void notifyTransform();

#pragma endregion
#pragma region Getters/Setters

        /**
         * Return the mesh of this object.
         */
        geometry::Mesh* getMesh() const;

        /**
         * Return a new mesh with the current transform applied to it. The caller is responsible for deleting the returned mesh.
         */
        geometry::Mesh* getTransformedMesh() const;

        /**
         * Return a COPY of the transform of this object. (view details below)
         * @details Modifying the returned Transform will NOT affect the object's actual transform.
         * To change the object's transform, use setTransform() with a modified Transform.
         */
        Transform getTransform() const;

        /**
         * Set the transform of this object and notify observers of the change.
         */
        Transform setTransform(const Transform t);

#pragma endregion
#pragma region Private

      private:
        geometry::Mesh* _mesh;
        Transform       _transform;

        std::vector<ObjectObserver*> observers;
    };

    // #pragma region BatchBuilder

    //     static std::vector<Object3D> batchBuildFromStl(const std::vector<std::string>& stlFiles)
    //     {
    //         std::vector<Object3D> objects;
    //         objects.reserve(stlFiles.size());
    //         for (const auto& file : stlFiles)
    //             objects.emplace_back(Object3D::fromSTL(file));
    //         return objects;
    //     }

    //     static std::vector<Object3D> batchBuildFromMeshes(const std::vector<geometry::Mesh>& meshes)
    //     {
    //         std::vector<Object3D> objects;
    //         objects.reserve(meshes.size());
    //         for (const auto& mesh : meshes)
    //             objects.emplace_back(Object3D(new geometry::Mesh(mesh)));
    //         return objects;
    //     }

    // #pragma endregion
} // namespace objects3D
