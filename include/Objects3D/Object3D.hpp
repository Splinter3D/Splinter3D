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

        Object3D(geometry::Mesh* m)
            : _mesh(m)
        { }

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
         * Load the mesh from an STL file and create an Object3D.
         */
        inline static Object3D fromSTL(const std::string& stlFile)
        {
            return Object3D(new geometry::Mesh(geometry::Mesh::fromSTL(stlFile)));
        }

#pragma endregion
#pragma region Observers

        /**
         * Attaches an observer to this object. The observer will be notified when notifyTransform() or notifyAppearanceChanged() is called.
         */
        void attach(ObjectObserver* obs)
        {
            if (std::find(observers.begin(), observers.end(), obs) == observers.end())
                observers.push_back(obs);
        }

        /**
         * Notify all the observers that the transform has changed.
         */
        void notifyTransform()
        {
            for (auto* o : observers)
                o->onTransformChanged();
        }

#pragma endregion
#pragma region Getters/Setters

        /**
         * Return the mesh of this object.
         */
        geometry::Mesh* getMesh() const
        {
            return _mesh;
        }

        /**
         * Return a new mesh with the current transform applied to it. The caller is responsible for deleting the returned mesh.
         */
        geometry::Mesh* getTransformedMesh() const
        {
            geometry::Mesh* transformedMesh = new geometry::Mesh();
            Matrix          transformMatrix = _transform.toMatrix();

            for (const auto& tri : _mesh->triangles)
            {
                geometry::Triangle transformedTri;
                for (int i = 0; i < 3; ++i)
                {
                    transformedTri.vertices[i].x = transformMatrix.m0 * tri.vertices[i].x + transformMatrix.m4 * tri.vertices[i].y +
                                                   transformMatrix.m8 * tri.vertices[i].z + transformMatrix.m12;
                    transformedTri.vertices[i].y = transformMatrix.m1 * tri.vertices[i].x + transformMatrix.m5 * tri.vertices[i].y +
                                                   transformMatrix.m9 * tri.vertices[i].z + transformMatrix.m13;
                    transformedTri.vertices[i].z = transformMatrix.m2 * tri.vertices[i].x + transformMatrix.m6 * tri.vertices[i].y +
                                                   transformMatrix.m10 * tri.vertices[i].z + transformMatrix.m14;
                }
                transformedMesh->triangles.push_back(transformedTri);
            }
            return transformedMesh;
        }

        /**
         * Return a COPY of the transform of this object. (view details below)
         * @details Modifying the returned Transform will NOT affect the object's actual transform.
         * To change the object's transform, use setTransform() with a modified Transform.
         */
        Transform getTransform() const
        {
            return _transform;
        }

        /**
         * Set the transform of this object and notify observers of the change.
         */
        Transform setTransform(const Transform t)
        {
            _transform = t;
            notifyTransform();
            return _transform;
        }

#pragma endregion
#pragma region Private

      private:
        geometry::Mesh* _mesh;
        Transform       _transform;

        std::vector<ObjectObserver*> observers;
    };
} // namespace objects3D
