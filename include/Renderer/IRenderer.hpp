#pragma once

#include <Geometry/Mesh.hpp>
#include <Geometry/Triangle.hpp>
#include <Geometry/Vec3.hpp>
#include <Renderer/Color.hpp>
#include <Renderer/RenderObject.hpp>
#include <cstdint>
#include <string>
#include <vector>

namespace renderer
{
    struct Config
    {
        int         width{1920};
        int         height{1080};
        std::string title{"Renderer"};
        int         target_fps{60};
    };

    enum class Key
    {
        Unknown = -1,
        A,
        B,
        C,
        D,
        E,
        F,
        G,
        H,
        I,
        J,
        K,
        L,
        M,
        N,
        O,
        P,
        Q,
        R,
        S,
        T,
        U,
        V,
        W,
        X,
        Y,
        Z,
        Space,
        Escape,
        Enter,
        Shift,
        Ctrl,
        Alt,
        Tab,
        Backspace
    };

    /**
     * Interface for a 3D renderer suitable for slicers
     */
    class IRenderer
    {
      public:
        virtual ~IRenderer() = default;

        // Frame handling
        virtual float beginFrame() = 0;
        virtual void  endFrame()   = 0;

        // 3D rendering
        virtual void begin3D() = 0;
        virtual void end3D()   = 0;

        // Drawing primitives
        virtual void drawTriangle(const geometry::Triangle& tri, Color color) = 0;
        virtual void drawMesh(const geometry::Mesh& mesh, Color color)        = 0;

        virtual void drawObject(const RenderObject& obj, Color color) = 0;

        virtual void drawGrid(int slices, float spacing) = 0;
        virtual void drawAxis(float size)                = 0;

        // Input / window
        virtual bool shouldClose() const      = 0;
        virtual void requestClose()           = 0;
        virtual bool isKeyDown(Key key) const = 0;
    };
} // namespace renderer
