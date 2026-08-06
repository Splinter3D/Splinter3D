#include <Objects3D/Object3D.hpp>

namespace objects3D
{
    Object3D::Object3D(geometry::Mesh* m)
        : _mesh(m)
    { }

    Object3D::Object3D(const Object3D& other)
    {
        _mesh      = new geometry::Mesh(*other._mesh);
        _transform = other._transform;
    }

    Object3D::~Object3D()
    {
        delete _mesh;
    }

    Object3D Object3D::fromSTL(const std::string& stlFile)
    {
        return Object3D(new geometry::Mesh(geometry::Mesh::fromSTL(stlFile)));
    }

#pragma endregion
#pragma region Observers

    void Object3D::attach(ObjectObserver* obs)
    {
        if (std::find(observers.begin(), observers.end(), obs) == observers.end())
            observers.push_back(obs);
    }

    void Object3D::detach(ObjectObserver* obs)
    {
        observers.erase(std::remove(observers.begin(), observers.end(), obs), observers.end());
    }

    void Object3D::notifyTransform()
    {
        for (auto* o : observers)
            o->onTransformChanged();
    }

#pragma endregion
#pragma region Getters/Setters

    geometry::Mesh* Object3D::getMesh() const
    {
        return _mesh;
    }

    geometry::Mesh* Object3D::getTransformedMesh() const
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

    Transform Object3D::getTransform() const
    {
        return _transform;
    }

    Transform Object3D::setTransform(const Transform t)
    {
        _transform = t;
        notifyTransform();
        return _transform;
    }

} // namespace objects3D
