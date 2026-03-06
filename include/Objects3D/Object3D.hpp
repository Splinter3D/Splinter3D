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
         * Create a new Object3D by copying the mesh and transform of another Object3D. The observers are NOT copied.
         */
        Object3D(const Object3D& other)
        {
            _mesh      = new geometry::Mesh(*other._mesh);
            _transform = other._transform;
        }

        ~Object3D()
        {
            delete _mesh;
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
         * Detaches an observer from this object. The observer will no longer receive notifications.
         */
        void detach(ObjectObserver* obs)
        {
            observers.erase(std::remove(observers.begin(), observers.end(), obs), observers.end());
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
