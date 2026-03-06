#pragma once

#include <Geometry/Mesh.hpp>
#include <Geometry/Ray.hpp>
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
        // Letters — values match Raylib
        A         = 65,
        B         = 66,
        C         = 67,
        D         = 68,
        E         = 69,
        F         = 70,
        G         = 71,
        H         = 72,
        I         = 73,
        J         = 74,
        K         = 75,
        L         = 76,
        M         = 77,
        N         = 78,
        O         = 79,
        P         = 80,
        Q         = 81,
        R         = 82,
        S         = 83,
        T         = 84,
        U         = 85,
        V         = 86,
        W         = 87,
        X         = 88,
        Y         = 89,
        Z         = 90,
        Space     = 32,
        Escape    = 256,
        Enter     = 257,
        Tab       = 258,
        Backspace = 259,
        Suppr     = 261,
        Shift     = 340,
        Ctrl      = 341,
        Alt       = 342,
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
        virtual void      drawGuiComponent(const gui::IGuiComponent& component) const                                                                                                          = 0;
        virtual ITexture* createIcon(int width, int height, const std::function<void(void* canvas)>& painter)                                                                                  = 0;
        virtual void      drawRectangleLines(float x, float y, float width, float height, Color color, Layer layer = Layer::UI) const                                                          = 0;
        virtual void      drawRectangle(float x, float y, float width, float height, Color color, Layer layer = Layer::UI) const                                                               = 0;
        virtual void      drawValueBox(float x, float y, float width, float height, const char* label, int& value, int min, int max, bool& editMode, Layer layer = Layer::UI) const            = 0;
        virtual void      drawFloatValueBox(float x, float y, float width, float height, const char* label, float& value, float min, float max, bool& editMode, Layer layer = Layer::UI) const = 0;
        virtual void      drawCheckbox(float x, float y, float size, const char* label, bool& checked, Layer layer = Layer::UI) const                                                          = 0;
        virtual void      drawTexture(float x, float y, float width, float height, const ITexture* texture, Layer layer = Layer::UI) const                                                     = 0;
        virtual void      drawButton(float x, float y, float width, float height, const ITexture* icon, const std::function<void()>& onClick, Layer layer = Layer::UI) const                   = 0;
        virtual void      drawPanel(float x, float y, float width, float height, Layer layer = Layer::Overlay) const                                                                           = 0;
        virtual void      drawText(float x, float y, const char* text, int fontSize, Layer layer = Layer::UI) const                                                                            = 0;
        virtual float     measureTextWidth(const char* text, int fontSize) const                                                                                                               = 0;

        virtual void* getCanvas() const = 0; // For icon drawing

        // Specific icon painters
        virtual void drawImportIcon(void* canvas)    = 0;
        virtual void drawExportIcon(void* canvas)    = 0;
        virtual void drawSliceIcon(void* canvas)     = 0;
        virtual void drawScaleIcon(void* canvas)     = 0;
        virtual void drawTransformIcon(void* canvas) = 0;
        virtual void drawPreviewIcon(void* canvas)   = 0;
        virtual void drawRotationIcon(void* canvas)  = 0;

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
        virtual geometry::Ray  getMouseRay() const                    = 0;
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
