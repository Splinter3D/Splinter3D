#pragma once

// ─────────────────────────────────────────────
//  SkyboxRenderer
//
//  Skybox OpenGL pur (cube map procédural — pas de texture externe).
//  Dessiné avant OCCT AIS pour servir de fond 3D.
//
//  Usage:
//    SkyboxRenderer sky;
//    sky.init();                   // une fois après SetCurrent()
//    sky.draw(viewMatrix, projMatrix);  // avant m_view->Redraw()
// ─────────────────────────────────────────────

// ── Ordre critique : OCCT OpenGL AVANT GL système ────────────────────────────
// OCCT embarque son propre glext.h (2014) — doit être inclus en premier
// pour éviter les conflits de typedef avec /usr/include/GL/glext.h (2019).
#include <OpenGl_GraphicDriver.hxx>

// GL système — après OCCT (les guards #pragma once / #ifdef évitent la redéf)
#include <GL/glx.h>
#include <array>
#include <cmath>
#include <string>

// Macros pour charger les fonctions GL modernes (OpenGL 3.3 core)
// On les charge manuellement pour éviter une dépendance à GLEW/GLAD
namespace detail
{
    inline PFNGLCREATESHADERPROC            glCreateShader_            = nullptr;
    inline PFNGLSHADERSOURCEPROC            glShaderSource_            = nullptr;
    inline PFNGLCOMPILESHADERPROC           glCompileShader_           = nullptr;
    inline PFNGLCREATEPROGRAMPROC           glCreateProgram_           = nullptr;
    inline PFNGLATTACHSHADERPROC            glAttachShader_            = nullptr;
    inline PFNGLLINKPROGRAMPROC             glLinkProgram_             = nullptr;
    inline PFNGLUSEPROGRAMPROC              glUseProgram_              = nullptr;
    inline PFNGLGETUNIFORMLOCATIONPROC      glGetUniformLocation_      = nullptr;
    inline PFNGLUNIFORMMATRIX4FVPROC        glUniformMatrix4fv_        = nullptr;
    inline PFNGLUNIFORM1IPROC               glUniform1i_               = nullptr;
    inline PFNGLGENVERTEXARRAYSPROC         glGenVertexArrays_         = nullptr;
    inline PFNGLBINDVERTEXARRAYPROC         glBindVertexArray_         = nullptr;
    inline PFNGLGENBUFFERSPROC              glGenBuffers_              = nullptr;
    inline PFNGLBINDBUFFERPROC              glBindBuffer_              = nullptr;
    inline PFNGLBUFFERDATAPROC              glBufferData_              = nullptr;
    inline PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray_ = nullptr;
    inline PFNGLVERTEXATTRIBPOINTERPROC     glVertexAttribPointer_     = nullptr;
    inline PFNGLDELETEVERTEXARRAYSPROC      glDeleteVertexArrays_      = nullptr;
    inline PFNGLDELETEBUFFERSPROC           glDeleteBuffers_           = nullptr;
    inline PFNGLDELETEPROGRAMPROC           glDeleteProgram_           = nullptr;
    inline PFNGLGETSHADERIVPROC             glGetShaderiv_             = nullptr;
    inline PFNGLGETSHADERINFOLOGPROC        glGetShaderInfoLog_        = nullptr;

    inline void* getProcAddr(const char* name)
    {
        return (void*) glXGetProcAddressARB((const GLubyte*) name);
    }

    inline void loadGLFunctions()
    {
        static bool loaded = false;
        if (loaded)
            return;
        loaded = true;
#define LOAD(fn, type) fn##_ = (type) getProcAddr(#fn)
        LOAD(glCreateShader, PFNGLCREATESHADERPROC);
        LOAD(glShaderSource, PFNGLSHADERSOURCEPROC);
        LOAD(glCompileShader, PFNGLCOMPILESHADERPROC);
        LOAD(glCreateProgram, PFNGLCREATEPROGRAMPROC);
        LOAD(glAttachShader, PFNGLATTACHSHADERPROC);
        LOAD(glLinkProgram, PFNGLLINKPROGRAMPROC);
        LOAD(glUseProgram, PFNGLUSEPROGRAMPROC);
        LOAD(glGetUniformLocation, PFNGLGETUNIFORMLOCATIONPROC);
        LOAD(glUniformMatrix4fv, PFNGLUNIFORMMATRIX4FVPROC);
        LOAD(glUniform1i, PFNGLUNIFORM1IPROC);
        LOAD(glGenVertexArrays, PFNGLGENVERTEXARRAYSPROC);
        LOAD(glBindVertexArray, PFNGLBINDVERTEXARRAYPROC);
        LOAD(glGenBuffers, PFNGLGENBUFFERSPROC);
        LOAD(glBindBuffer, PFNGLBINDBUFFERPROC);
        LOAD(glBufferData, PFNGLBUFFERDATAPROC);
        LOAD(glEnableVertexAttribArray, PFNGLENABLEVERTEXATTRIBARRAYPROC);
        LOAD(glVertexAttribPointer, PFNGLVERTEXATTRIBPOINTERPROC);
        LOAD(glDeleteVertexArrays, PFNGLDELETEVERTEXARRAYSPROC);
        LOAD(glDeleteBuffers, PFNGLDELETEBUFFERSPROC);
        LOAD(glDeleteProgram, PFNGLDELETEPROGRAMPROC);
        LOAD(glGetShaderiv, PFNGLGETSHADERIVPROC);
        LOAD(glGetShaderInfoLog, PFNGLGETSHADERINFOLOGPROC);
#undef LOAD
    }
} // namespace detail

// Aliases pratiques
#define GL_CreateShader            detail::glCreateShader_
#define GL_ShaderSource            detail::glShaderSource_
#define GL_CompileShader           detail::glCompileShader_
#define GL_CreateProgram           detail::glCreateProgram_
#define GL_AttachShader            detail::glAttachShader_
#define GL_LinkProgram             detail::glLinkProgram_
#define GL_UseProgram              detail::glUseProgram_
#define GL_GetUniformLocation      detail::glGetUniformLocation_
#define GL_UniformMatrix4fv        detail::glUniformMatrix4fv_
#define GL_Uniform1i               detail::glUniform1i_
#define GL_GenVertexArrays         detail::glGenVertexArrays_
#define GL_BindVertexArray         detail::glBindVertexArray_
#define GL_GenBuffers              detail::glGenBuffers_
#define GL_BindBuffer              detail::glBindBuffer_
#define GL_BufferData              detail::glBufferData_
#define GL_EnableVertexAttribArray detail::glEnableVertexAttribArray_
#define GL_VertexAttribPointer     detail::glVertexAttribPointer_
#define GL_DeleteVertexArrays      detail::glDeleteVertexArrays_
#define GL_DeleteBuffers           detail::glDeleteBuffers_
#define GL_DeleteProgram           detail::glDeleteProgram_
#define GL_GetShaderiv             detail::glGetShaderiv_
#define GL_GetShaderInfoLog        detail::glGetShaderInfoLog_

namespace wx::occt
{

    class SkyboxRenderer
    {
      public:
        ~SkyboxRenderer()
        {
            cleanup();
        }

        void init()
        {
            detail::loadGLFunctions();
            buildShader();
            buildCubeVAO();
            buildProceduralCubemap();
            m_ready = true;
        }

        // Appeler AVANT m_view->Redraw()
        // viewMat : matrice 4x4 float col-major (vue sans translation)
        // projMat : matrice 4x4 float col-major
        void draw(const float* viewMat, const float* projMat)
        {
            if (!m_ready)
                return;

            // Désactive le depth write — la skybox est toujours derrière tout
            glDepthMask(GL_FALSE);

            GL_UseProgram(m_shader);
            GL_UniformMatrix4fv(GL_GetUniformLocation(m_shader, "uView"), 1, GL_FALSE, viewMat);
            GL_UniformMatrix4fv(GL_GetUniformLocation(m_shader, "uProj"), 1, GL_FALSE, projMat);
            GL_Uniform1i(GL_GetUniformLocation(m_shader, "uSkybox"), 0);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubemap);

            GL_BindVertexArray(m_vao);
            glDrawArrays(GL_TRIANGLES, 0, 36);
            GL_BindVertexArray(0);

            glDepthMask(GL_TRUE);
            GL_UseProgram(0);
        }

      private:
        GLuint m_shader  = 0;
        GLuint m_vao     = 0;
        GLuint m_vbo     = 0;
        GLuint m_cubemap = 0;
        bool   m_ready   = false;

        // ── GLSL shaders ─────────────────────────────────────────────────────

        static constexpr const char* VERT_SRC = R"glsl(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        out vec3 vTexCoord;
        uniform mat4 uView;
        uniform mat4 uProj;
        void main()
        {
            vTexCoord   = aPos;
            vec4 pos    = uProj * uView * vec4(aPos, 1.0);
            gl_Position = pos.xyww; // force depth = 1.0 (always behind)
        }
    )glsl";

        static constexpr const char* FRAG_SRC = R"glsl(
        #version 330 core
        in  vec3      vTexCoord;
        out vec4      FragColor;
        uniform samplerCube uSkybox;
        void main()
        {
            FragColor = texture(uSkybox, vTexCoord);
        }
    )glsl";

        // ── Cube VAO ─────────────────────────────────────────────────────────

        void buildCubeVAO()
        {
            // clang-format off
        static const float verts[] = {
            -1, 1,-1, -1,-1,-1,  1,-1,-1,  1,-1,-1,  1, 1,-1, -1, 1,-1,
            -1,-1, 1, -1,-1,-1, -1, 1,-1, -1, 1,-1, -1, 1, 1, -1,-1, 1,
             1,-1,-1,  1,-1, 1,  1, 1, 1,  1, 1, 1,  1, 1,-1,  1,-1,-1,
            -1,-1, 1, -1, 1, 1,  1, 1, 1,  1, 1, 1,  1,-1, 1, -1,-1, 1,
            -1, 1,-1,  1, 1,-1,  1, 1, 1,  1, 1, 1, -1, 1, 1, -1, 1,-1,
            -1,-1,-1, -1,-1, 1,  1,-1,-1,  1,-1,-1, -1,-1, 1,  1,-1, 1,
        };
            // clang-format on
            GL_GenVertexArrays(1, &m_vao);
            GL_GenBuffers(1, &m_vbo);
            GL_BindVertexArray(m_vao);
            GL_BindBuffer(GL_ARRAY_BUFFER, m_vbo);
            GL_BufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
            GL_EnableVertexAttribArray(0);
            GL_VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*) 0);
            GL_BindVertexArray(0);
        }

        // ── Cube map procédural (dégradé ciel studio) ─────────────────────────

        void buildProceduralCubemap()
        {
            glGenTextures(1, &m_cubemap);
            glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubemap);

            // 6 faces, 64x64, couleur interpolée selon la direction
            const int                              SZ = 64;
            std::array<unsigned char, SZ * SZ * 3> pixels;

            // Couleurs par face : top=ciel clair, bottom=sol foncé, côtés=gris bleuté
            struct FaceColors
            {
                float r0, g0, b0, r1, g1, b1;
            };

            const FaceColors faces[6] = {
                {0.55f, 0.70f, 0.85f, 0.30f, 0.45f, 0.65f}, // +X
                {0.55f, 0.70f, 0.85f, 0.30f, 0.45f, 0.65f}, // -X
                {0.75f, 0.88f, 1.00f, 0.55f, 0.70f, 0.85f}, // +Y (top — plus clair)
                {0.20f, 0.20f, 0.25f, 0.10f, 0.10f, 0.15f}, // -Y (sol — sombre)
                {0.55f, 0.70f, 0.85f, 0.30f, 0.45f, 0.65f}, // +Z
                {0.55f, 0.70f, 0.85f, 0.30f, 0.45f, 0.65f}, // -Z
            };

            const GLenum targets[6] = {
                GL_TEXTURE_CUBE_MAP_POSITIVE_X,
                GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
                GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
                GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
                GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
                GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
            };

            for (int f = 0; f < 6; ++f)
            {
                const auto& fc = faces[f];
                for (int y = 0; y < SZ; ++y)
                {
                    float t = float(y) / float(SZ - 1);
                    for (int x = 0; x < SZ; ++x)
                    {
                        int idx         = (y * SZ + x) * 3;
                        pixels[idx + 0] = (unsigned char) ((fc.r0 * (1 - t) + fc.r1 * t) * 255);
                        pixels[idx + 1] = (unsigned char) ((fc.g0 * (1 - t) + fc.g1 * t) * 255);
                        pixels[idx + 2] = (unsigned char) ((fc.b0 * (1 - t) + fc.b1 * t) * 255);
                    }
                }
                glTexImage2D(targets[f], 0, GL_RGB, SZ, SZ, 0,
                             GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
            }

            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        }

        // ── Compilation shader ────────────────────────────────────────────────

        void buildShader()
        {
            auto compile = [](GLenum type, const char* src) -> GLuint {
                GLuint s = GL_CreateShader(type);
                GL_ShaderSource(s, 1, &src, nullptr);
                GL_CompileShader(s);
                GLint ok;
                GL_GetShaderiv(s, GL_COMPILE_STATUS, &ok);
                if (!ok)
                {
                    char log[512];
                    GL_GetShaderInfoLog(s, 512, nullptr, log);
                    fprintf(stderr, "Shader error: %s\n", log);
                }
                return s;
            };

            GLuint vs = compile(GL_VERTEX_SHADER, VERT_SRC);
            GLuint fs = compile(GL_FRAGMENT_SHADER, FRAG_SRC);
            m_shader  = GL_CreateProgram();
            GL_AttachShader(m_shader, vs);
            GL_AttachShader(m_shader, fs);
            GL_LinkProgram(m_shader);
        }

        void cleanup()
        {
            if (m_vao)
            {
                GL_DeleteVertexArrays(1, &m_vao);
                m_vao = 0;
            }
            if (m_vbo)
            {
                GL_DeleteBuffers(1, &m_vbo);
                m_vbo = 0;
            }
            if (m_cubemap)
            {
                glDeleteTextures(1, &m_cubemap);
                m_cubemap = 0;
            }
            if (m_shader)
            {
                GL_DeleteProgram(m_shader);
                m_shader = 0;
            }
        }
    };

} // namespace wx::occt