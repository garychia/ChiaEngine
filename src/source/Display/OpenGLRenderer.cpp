// DEPRECATED — OpenGL backend. See OpenGLRenderer.hpp: not wired into the
// Frame/IFrameExecutor architecture, kept only as the non-Vulkan CMake
// fallback. GUI TODOs (:212 LoadGUILayout, :309 Render(GUILayout&)) are
// intentionally NOT implemented — the GLFW window path is Frame-driven and
// cannot reach them; see docs/agents/opengl-backend-assessment.md.
#include "Display/OpenGLRenderer.hpp"
#include "Display/Window.hpp"
#include "System/Debug/Debug.hpp"
#include "System/IO/IO.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "3rdparty/stb_image.h"

// ─── Default GLSL shaders (embedded) ────────────────────────────────────────

static const char DefaultVertexShader[] = R"glsl(
#version 330 core

layout (std140) uniform MatrixBlock
{
    mat4 world;
    mat4 view;
    mat4 projection;
};

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in uint aCMode;
layout (location = 4) in uint aGUI;

out vec4 vColor;
out vec2 vTexCoord;
flat out uint vCMode;
flat out uint vGUI;

void main()
{
    gl_Position = projection * view * world * vec4(aPos, 1.0);
    vColor     = aColor;
    vTexCoord  = aTexCoord;
    vCMode     = aCMode;
    vGUI       = aGUI;
}
)glsl";

static const char DefaultFragmentShader[] = R"glsl(
#version 330 core

in vec4 vColor;
in vec2 vTexCoord;
flat in uint vCMode;
flat in uint vGUI;

out vec4 FragColor;

uniform sampler2D uTexture;
uniform bool uUseTexture = false;

void main()
{
    vec4 base = vColor;
    if (uUseTexture)
    {
        vec4 texel = texture(uTexture, vTexCoord);
        // vCMode & 1 → modulate
        if ((vCMode & 1u) != 0u)
            base *= texel;
        else
            base = texel;
    }
    FragColor = base;
}
)glsl";

// ─── Implementation ─────────────────────────────────────────────────────────

OpenGLRenderer::OpenGLRenderer()
{
}

OpenGLRenderer::~OpenGLRenderer()
{
    Clear();
    if (shaderProgram)
        glDeleteProgram(shaderProgram);
    glDeleteBuffers(1, &uboMatrix);
    OpenGLHelper::DestroyWindow(pWindow);
}

bool OpenGLRenderer::Initialize(const Window *pWindow)
{
    if (!OpenGLHelper::InitGLFW())
        return false;

    OpenGLHelper::CreateWindow(this->pWindow,
                                this->windowWidth,
                                this->windowHeight,
                                "ChiaEngine (OpenGL)");
    if (!this->pWindow)
        return false;

    CreateDefaultShader();
    return true;
}

void OpenGLRenderer::CreateDefaultShader()
{
    GLuint vert = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert, 1, &DefaultVertexShader, nullptr);
    glCompileShader(vert);
    {
        GLint ok = 0;
        glGetShaderiv(vert, GL_COMPILE_STATUS, &ok);
        if (!ok)
        {
            char buf[1024];
            glGetShaderInfoLog(vert, sizeof(buf), nullptr, buf);
            PRINTLN_ERR("OpenGLRenderer: default vertex shader compile failed:\n" << buf);
            glDeleteShader(vert);
            return;
        }
    }

    GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag, 1, &DefaultFragmentShader, nullptr);
    glCompileShader(frag);
    {
        GLint ok = 0;
        glGetShaderiv(frag, GL_COMPILE_STATUS, &ok);
        if (!ok)
        {
            char buf[1024];
            glGetShaderInfoLog(frag, sizeof(buf), nullptr, buf);
            PRINTLN_ERR("OpenGLRenderer: default fragment shader compile failed:\n" << buf);
            glDeleteShader(vert);
            glDeleteShader(frag);
            return;
        }
    }

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vert);
    glAttachShader(shaderProgram, frag);
    glLinkProgram(shaderProgram);
    {
        GLint ok = 0;
        glGetProgramiv(shaderProgram, GL_LINK_STATUS, &ok);
        if (!ok)
        {
            char buf[1024];
            glGetProgramInfoLog(shaderProgram, sizeof(buf), nullptr, buf);
            PRINTLN_ERR("OpenGLRenderer: default program link failed:\n" << buf);
            glDeleteProgram(shaderProgram);
            shaderProgram = 0;
        }
    }
    glDeleteShader(vert);
    glDeleteShader(frag);

    if (shaderProgram)
    {
        glUseProgram(shaderProgram);
        GLuint blockIdx = glGetUniformBlockIndex(shaderProgram, "MatrixBlock");
        if (blockIdx != GL_INVALID_INDEX)
        {
            glUniformBlockBinding(shaderProgram, blockIdx, 0);
            glGenBuffers(1, &uboMatrix);
            glBindBuffer(GL_UNIFORM_BUFFER, uboMatrix);
            glBufferData(GL_UNIFORM_BUFFER, sizeof(MatrixBuffer), nullptr, GL_DYNAMIC_DRAW);
            glBindBufferBase(GL_UNIFORM_BUFFER, 0, uboMatrix);
            glBindBuffer(GL_UNIFORM_BUFFER, 0);
        }
        glUseProgram(0);
    }

    glGenVertexArrays(1, &defaultVAO);

    PRINTLN("OpenGLRenderer: default shader created (program=" << shaderProgram << ").");
}

bool OpenGLRenderer::SwitchToFullScreen()
{
    // GLFW fullscreen toggle — recreate window with monitor resolution
    GLFWmonitor *monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode *mode = glfwGetVideoMode(monitor);
    glfwSetWindowMonitor(pWindow, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    windowWidth = mode->width;
    windowHeight = mode->height;
    return true;
}

bool OpenGLRenderer::SwitchToWindowMode()
{
    glfwSetWindowMonitor(pWindow, nullptr, 100, 100, 800, 600, GLFW_DONT_CARE);
    windowWidth = 800;
    windowHeight = 600;
    return true;
}

bool OpenGLRenderer::LoadScene(Scene &scene)
{
    for (size_t i = 0; i < scene.GetRenderables().Length(); i++)
    {
        const SharedPtr<IRenderable> &pRenderable = scene.GetRenderables()[i];
        if (!pRenderable->GetIdentifier())
            continue;
        IRenderable &renderable = *pRenderable;
        LoadRenderable(renderable, scene.GetType());
    }
    return true;
}

bool OpenGLRenderer::LoadGUILayout(GUILayout &layout)
{
    (void)layout;
    // TODO: GUI support
    return true;
}

bool OpenGLRenderer::AddVertexShader(Shader &shader)
{
    if (shader.loaded)
        return true;
    // Load from file and mark the shader's identifier
    GLuint glShader = OpenGLHelper::CompileShader(shader.path, GL_VERTEX_SHADER);
    if (!glShader)
        return false;
    shader.loaded = true;
    shader.identifier = glShader;
    return true;
}

bool OpenGLRenderer::AddPixelShader(Shader &shader)
{
    if (shader.loaded)
        return true;
    GLuint glShader = OpenGLHelper::CompileShader(shader.path, GL_FRAGMENT_SHADER);
    if (!glShader)
        return false;
    shader.loaded = true;
    shader.identifier = glShader;
    return true;
}

void OpenGLRenderer::ApplyCamera(WeakPtr<Camera> pCamera)
{
    this->pCamera = pCamera;
    OnCameraChanged();
}

void OpenGLRenderer::OnCameraChanged()
{
    cameraChanged = true;
}

void OpenGLRenderer::OnWindowResized(long newWidth, long newHeight)
{
    windowWidth = static_cast<int>(newWidth);
    windowHeight = static_cast<int>(newHeight);
    if (pWindow)
        glfwSetWindowSize(pWindow, windowWidth, windowHeight);
    glViewport(0, 0, windowWidth, windowHeight);
    cameraChanged = true;
}

void OpenGLRenderer::Update()
{
    glfwPollEvents();

    if (cameraChanged && pCamera)
    {
        Camera &cam = *pCamera;
        viewMatrix = OpenGLHelper::BuildViewMatrix(cam.GetPosition(), cam.GetRotation());
        float aspect = static_cast<float>(windowWidth) / static_cast<float>(windowHeight);
        projMatrix = OpenGLHelper::BuildProjMatrix(cam.GetAngleOfView(), aspect,
                                                    cam.GetDistanceToNearPlane(),
                                                    cam.GetDistanceToFarPlane());
        cameraChanged = false;
    }

    // Update matrix UBO with identity when no camera
    if (!pCamera)
    {
        viewMatrix = glm::mat4(1.0f);
        float aspect = static_cast<float>(windowWidth) / static_cast<float>(windowHeight);
        projMatrix = glm::perspective(glm::radians(70.0f), aspect, 0.001f, 100.0f);
    }
}

void OpenGLRenderer::Render(Scene &scene)
{
    if (!shaderProgram)
        return;

    glUseProgram(shaderProgram);
    glBindVertexArray(defaultVAO);

    for (size_t i = 0; i < scene.GetRenderables().Length(); i++)
    {
        const SharedPtr<IRenderable> &pRenderable = scene.GetRenderables()[i];
        if (!pRenderable->GetIdentifier())
            continue;
        RenderRenderable(*pRenderable);
    }

    glBindVertexArray(0);
    glUseProgram(0);
}

void OpenGLRenderer::Render(GUILayout &layout)
{
    (void)layout;
    // TODO: GUI rendering
}

void OpenGLRenderer::Clear()
{
    vertexBuffers.RemoveAll();
    indexBuffers.RemoveAll();
    vertexArrays.RemoveAll();
    textures.RemoveAll();
    textureUnits.RemoveAll();
    indexCounts.RemoveAll();

    glUseProgram(0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

// ─── Internal helpers ────────────────────────────────────────────────────────

bool OpenGLRenderer::LoadRenderable(IRenderable &renderable, Scene::SceneType sceneType)
{
    if (renderable.IsLoaded())
        return true;

    RenderInfo info = renderable.GetRenderInfo();

    // Load texture if provided
    if (info.pTexture && !info.pTexture->loaded)
    {
        int w = 0, h = 0;
        GLuint glTex = OpenGLHelper::CreateTexture(info.pTexture->path, w, h);
        if (!glTex)
        {
            PRINTLN_ERR("OpenGLRenderer: failed to load texture.");
            return false;
        }
        info.pTexture->loaded = true;
        info.pTexture->identifier = textures.Length();
        textures.Append(glTex);
    }

    // Build CPU-side vertex buffer
    VertexInfo *vertexData = CreateInputBuffer(renderable, sceneType);
    if (!vertexData)
        return false;

    // Create VAO, VBO, EBO
    GLuint VAO = 0, VBO = 0, EBO = 0;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);

    // Vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(VertexInfo) * info.numOfVertices,
                 vertexData, GL_STATIC_DRAW);
    delete[] vertexData;

    // Index buffer
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 sizeof(unsigned short) * info.numOfVertexIndices,
                 info.vertexIndexBuffer, GL_STATIC_DRAW);

    // Vertex attributes
    // aPos (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexInfo), (void *)offsetof(VertexInfo, position));
    glEnableVertexAttribArray(0);
    // aColor (location 1)
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(VertexInfo), (void *)offsetof(VertexInfo, color));
    glEnableVertexAttribArray(1);
    // aTexCoord (location 2)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VertexInfo), (void *)offsetof(VertexInfo, texCoord));
    glEnableVertexAttribArray(2);
    // aCMode (location 3)
    glVertexAttribIPointer(3, 1, GL_UNSIGNED_INT, sizeof(VertexInfo), (void *)offsetof(VertexInfo, cmode));
    glEnableVertexAttribArray(3);
    // aGUI (location 4)
    glVertexAttribIPointer(4, 1, GL_UNSIGNED_INT, sizeof(VertexInfo), (void *)offsetof(VertexInfo, gui));
    glEnableVertexAttribArray(4);

    glBindVertexArray(0);

    renderable.MarkLoaded(indexBuffers.Length());
    vertexArrays.Append(VAO);
    vertexBuffers.Append(VBO);
    indexBuffers.Append(EBO);
    indexCounts.Append(info.numOfVertexIndices);
    return true;
}

void OpenGLRenderer::RenderRenderable(IRenderable &renderable)
{
    if (!renderable.IsLoaded())
        return;

    const size_t id = renderable.GetIdentifier();
    if (id >= vertexArrays.Length())
        return;

    const RenderInfo &info = renderable.GetRenderInfo();

    // Update world matrix
    Point3D pos = renderable.GetPosition();
    Point3D rot = renderable.GetRotation();
    Point3D scale = renderable.GetScale();

    MatrixBuffer matrices;
    matrices.world = OpenGLHelper::BuildWorldMatrix(pos, rot, scale);
    matrices.view = viewMatrix;
    matrices.projection = projMatrix;

    glBindBuffer(GL_UNIFORM_BUFFER, uboMatrix);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(MatrixBuffer), &matrices);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // Bind texture
    if (info.pTexture && info.pTexture->loaded && info.pTexture->identifier < textures.Length())
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textures[info.pTexture->identifier]);
        glUniform1i(glGetUniformLocation(shaderProgram, "uTexture"), 0);
        glUniform1i(glGetUniformLocation(shaderProgram, "uUseTexture"), GL_TRUE);
    }
    else
    {
        glUniform1i(glGetUniformLocation(shaderProgram, "uUseTexture"), GL_FALSE);
    }

    // Draw
    glBindVertexArray(vertexArrays[id]);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indexCounts[id]),
                   GL_UNSIGNED_SHORT, nullptr);
    glBindVertexArray(0);
}

void OpenGLRenderer::UpdateMatrixUniforms(const Point3D &pos, const Point3D &rot, const Point3D &scale)
{
    MatrixBuffer matrices;
    matrices.world = OpenGLHelper::BuildWorldMatrix(pos, rot, scale);
    matrices.view = viewMatrix;
    matrices.projection = projMatrix;

    glBindBuffer(GL_UNIFORM_BUFFER, uboMatrix);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(MatrixBuffer), &matrices);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

VertexInfo *OpenGLRenderer::CreateInputBuffer(const IRenderable &renderable, Scene::SceneType sceneType)
{
    const RenderInfo info = renderable.GetRenderInfo();
    const auto nVertices = info.numOfVertices;
    if (!nVertices)
        return nullptr;

    VertexInfo *buffer = new VertexInfo[nVertices];

    // Use vertex index buffer if available, otherwise direct
    const bool useIndex = info.vertexIndexBuffer && info.numOfVertexIndices > 0;

    for (size_t i = 0; i < nVertices; i++)
    {
        size_t srcIdx = useIndex && i < info.numOfVertexIndices ? info.vertexIndexBuffer[i] : i;
        if (srcIdx >= info.numOfVertices)
            srcIdx = i;

        const Point3D &v = info.vertexBuffer[srcIdx];
        buffer[i].position = glm::vec3(v.X, v.Y, v.Z);

        if (info.colorBuffer && info.numOfColors > 0)
        {
            size_t cIdx = info.colorIndexBuffer && i < info.numOfColorIndices ? info.colorIndexBuffer[i] : srcIdx;
            if (cIdx >= info.numOfColors)
                cIdx = 0;
            const Color &c = info.colorBuffer[cIdx];
            buffer[i].color = glm::vec4(c.R, c.G, c.B, c.A);
        }
        else
        {
            buffer[i].color = glm::vec4(1.0f);
        }

        if (info.textureCoordinates && i < info.numOfTextureCoordinates)
        {
            buffer[i].texCoord = glm::vec2(info.textureCoordinates[i].X, info.textureCoordinates[i].Y);
        }
        else
        {
            buffer[i].texCoord = glm::vec2(0.0f);
        }

        buffer[i].cmode = 1; // modulate by default
        buffer[i].gui = (sceneType == Scene::SceneType::GUI) ? 1 : 0;
    }

    return buffer;
}