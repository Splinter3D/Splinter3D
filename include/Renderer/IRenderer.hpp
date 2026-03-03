#pragma once

#include <Geometry/Mesh.hpp>
#include <Geometry/Triangle.hpp>
#include <Geometry/Vec3.hpp>
#include <Gui/IGuiComponent.hpp>
#include <Renderer/Color.hpp>
#include <Renderer/ITexture.hpp>
#include <Renderer/RenderObject.hpp>
#include <cstdint>
#include <functional>
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

    enum class MouseButton
    {
        Left   = 0,
        Right  = 1,
        Middle = 2
    };

    enum class Layer : uint8_t
    {
        World   = 0, // 3D content
        UI      = 1, // button backgrounds, icons
        Overlay = 2, // hover tints, tooltips, panels
        Debug   = 3  // debug overlays
    };

    /**
     * Interface for a 3D renderer suitable for slicers
     */
    class IRenderer
    {
      public:
        // Frame handling
        virtual float beginFrame() = 0;
        virtual void  endFrame()   = 0;

        // 3D rendering
        virtual void begin3D() = 0;
        virtual void end3D()   = 0;

        // GUI drawing
        virtual void      drawGuiComponent(const gui::IGuiComponent& component) const                                                                                        = 0;
        virtual ITexture* createIcon(int width, int height, const std::function<void(void* canvas)>& painter)                                                                = 0;
        virtual void      drawTexture(const ITexture* texture, float x, float y, float width, float height, Layer layer = Layer::UI) const                                   = 0;
        virtual void      drawButton(float x, float y, float width, float height, const ITexture* icon, const std::function<void()>& onClick, Layer layer = Layer::UI) const = 0;
        virtual void      drawPanel(float x, float y, float w, float h, Layer layer = Layer::Overlay) const                                                                  = 0;
        virtual void      drawText(const char* text, float x, float y, int fontSize, Layer layer = Layer::UI) const                                                          = 0;
        virtual void      drawRectangle(float x, float y, float width, float height, Color color, Layer layer = Layer::UI) const                                             = 0;
        virtual void      drawRectangleLines(float x, float y, float width, float height, Color color, Layer layer = Layer::UI) const                                        = 0;
        virtual float     measureTextWidth(const char* text, int fontSize) const                                                                                             = 0;

        virtual void* getCanvas() const = 0; // For icon drawing

        // Specific icon painters
        virtual void drawImportIcon(void* canvas)  = 0;
        virtual void drawExportIcon(void* canvas)  = 0;
        virtual void drawSliceIcon(void* canvas)   = 0;
        virtual void drawScaleIcon(void* canvas)   = 0;
        virtual void drawPreviewIcon(void* canvas) = 0;

        // 3D Drawing
        virtual void drawTriangle(const geometry::Triangle& tri, Color color, Layer layer = Layer::World) = 0;
        virtual void drawMesh(const geometry::Mesh& mesh, Color color, Layer layer = Layer::World)        = 0;
        virtual void drawObject(const RenderObject& obj, Color color, Layer layer = Layer::World)         = 0;
        virtual void drawGrid(int slices, float spacing, Layer layer = Layer::World)                      = 0;
        virtual void drawAxis(float size, Layer layer = Layer::World)                                     = 0;

        // Camera control
        virtual void  updateCamera(float deltaTime)                     = 0;
        virtual void  setCameraPosition(const geometry::Vec3& position) = 0;
        virtual void  setCameraTarget(const geometry::Vec3& target)     = 0;
        virtual void  setCameraFov(float fovY)                          = 0;
        virtual float getCameraFov() const                              = 0;
        virtual void  setOrbitDistance(float d)                         = 0;
        virtual void  setOrbitAngles(float yaw, float pitch)            = 0;

        // Input / window
        virtual bool           shouldClose() const                    = 0;
        virtual int            getScreenWidth() const                 = 0;
        virtual int            getScreenHeight() const                = 0;
        virtual void           requestClose()                         = 0;
        virtual bool           isKeyDown(Key key) const               = 0;
        virtual bool           isKeyPressed(Key key) const            = 0;
        virtual bool           isMouseButtonDown(int button) const    = 0;
        virtual bool           isMouseButtonPressed(int button) const = 0;
        virtual geometry::Vec3 getMousePosition() const               = 0;
        virtual geometry::Vec3 getMouseDelta() const                  = 0;
        virtual float          getDeltaTime() const                   = 0;

        virtual float getMouseX() const
        {
            return getMousePosition().x;
        }

        virtual float getMouseY() const
        {
            return getMousePosition().y;
        }
    };
} // namespace renderer
