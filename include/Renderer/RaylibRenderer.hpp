#pragma once

#include <Renderer/IRenderer.hpp>
#include <map>
#include <memory>
#include <variant>
#include <vector>

namespace renderer
{
#pragma region Draw CMD

    struct LineCmd
    {
        geometry::Vec3 start;
        geometry::Vec3 end;
        Color          color;
    };

    struct GridCmd
    {
        int   slices;
        float spacing;
    };

    struct TriangleCmd
    {
        geometry::Triangle tri;
        Color              color;
        Matrix             modelMatrix;
    };

    struct RectCmd
    {
        float x, y, w, h;
        Color color;
    };

    struct RectLinesCmd
    {
        float x, y, w, h;
        Color color;
    };

    struct TextureCmd
    {
        const ITexture* texture;
        Rectangle       src;
        Rectangle       dest;
    };

    struct TextCmd
    {
        std::string text;
        int         x, y;
        int         fontSize;
        Color       color;
    };

    struct ValueBoxCmd
    {
        float       x, y, w, h;
        std::string label;
        int         value;
        int         min, max;
        bool        editMode;
        int*        outValue;
        bool*       outEdit;
    };

    struct ValueBoxFloatCmd
    {
        float       x, y, w, h;
        std::string label;
        float       value;
        float       min, max;
        bool        editMode;
        float*      outValue;
        bool*       outEdit;
    };

    struct CheckboxCmd
    {
        float       x, y, size;
        std::string label;
        bool        checked;
        bool*       outChecked;
    };

    using DrawCmd = std::variant<RectCmd, RectLinesCmd, TextureCmd, TextCmd, LineCmd, GridCmd, TriangleCmd, ValueBoxCmd, ValueBoxFloatCmd, CheckboxCmd>;

#pragma endregion
#pragma region Texture

    /**
     * Allow us to store raylib textures in our renderer without exposing raylib headers in the public interface.
     */
    struct RaylibTexture : ITexture
    {
        Texture2D tex{};
    };

#pragma endregion
#pragma region RenderObject

    class RaylibRenderer final : public IRenderer
    {
      public:
        explicit RaylibRenderer(const Config& cfg = {});
        ~RaylibRenderer();

        mutable std::map<Layer, std::vector<DrawCmd>> drawQueue_;

        // --------------------
        // Frame lifecycle
        // --------------------
        float beginFrame() override;
        void  endFrame() override;
        void  begin3D() override;
        void  end3D() override;

        // --------------------
        // Drawing Gui
        // --------------------
        void      drawGuiComponent(const gui::IGuiComponent& component) const override;
        ITexture* createIcon(int width, int height, const std::function<void(void* canvas)>& painter) override;
        void      drawRectangleLines(float x, float y, float width, float height, Color color, Layer layer = Layer::UI) const override;
        void      drawRectangle(float x, float y, float width, float height, Color color, Layer layer = Layer::UI) const override;
        void      drawValueBox(float x, float y, float width, float height, const char* label, int& value, int min, int max, bool& editMode, Layer layer = Layer::UI) const override;
        void      drawFloatValueBox(float x, float y, float width, float height, const char* label, float& value, float min, float max, bool& editMode, Layer layer = Layer::UI) const override;
        void      drawCheckbox(float x, float y, float size, const char* label, bool& checked, Layer layer = Layer::UI) const override;
        void      drawTexture(float x, float y, float width, float height, const ITexture* texture, Layer layer = Layer::UI) const override;
        void      drawButton(float x, float y, float width, float height, const ITexture* icon, const std::function<void()>& onClick, Layer layer = Layer::UI) const override;
        void      drawPanel(float x, float y, float width, float height, Layer layer = Layer::Overlay) const override;
        void      drawText(float x, float y, const char* text, int fontSize, Layer layer = Layer::UI) const override;
        float     measureTextWidth(const char* text, int fontSize) const override;
        void*     getCanvas() const override;

        // Specific icon painters
        void drawImportIcon(void* canvas) override;
        void drawExportIcon(void* canvas) override;
        void drawSliceIcon(void* canvas) override;
        void drawScaleIcon(void* canvas) override;
        void drawPreviewIcon(void* canvas) override;

        // --------------------
        // Drawing 3D
        // --------------------
        void drawTriangle(const geometry::Triangle& tri, Color color, Layer layer = Layer::World) override;
        void drawMesh(const geometry::Mesh& mesh, Color color, Layer layer = Layer::World) override;
        void drawObject(const RenderObject& obj, Color color, Layer layer = Layer::World) override;
        void drawGrid(int slices, float spacing, Layer layer = Layer::World) override;
        void drawAxis(float size, Layer layer = Layer::World) override;

        // --------------------
        // Camera
        // --------------------
        void  updateCamera(float deltaTime) override;
        void  setCameraPosition(const geometry::Vec3& position) override;
        void  setCameraTarget(const geometry::Vec3& target) override;
        void  setCameraFov(float fovY) override;
        float getCameraFov() const override;
        void  setOrbitDistance(float d) override;
        void  setOrbitAngles(float yaw, float pitch) override;

        // --------------------
        // Window / input
        // --------------------
        bool           shouldClose() const override;
        void           requestClose() override;
        int            getScreenWidth() const override;
        int            getScreenHeight() const override;
        bool           isKeyDown(Key key) const override;
        bool           isKeyPressed(Key key) const override;
        bool           isMouseButtonDown(int button) const override;
        bool           isMouseButtonPressed(int button) const override;
        geometry::Vec3 getMousePosition() const override;
        geometry::Vec3 getMouseDelta() const override;
        float          getDeltaTime() const override;
        geometry::Ray  getMouseRay() const override;

#pragma endregion
#pragma region Private

      private:
        struct Impl;
        std::unique_ptr<Impl> impl_;

        void ensureCCW(geometry::Triangle& tri, geometry::Vec3 cameraPos);
    };
} // namespace renderer
