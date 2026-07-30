#ifndef OPENGL_RENDERER_HPP
#define OPENGL_RENDERER_HPP

#include "Display/IRenderer.hpp"
#include "Display/Camera.hpp"
#include "Display/Scene.hpp"
#include "Display/Shader.hpp"
#include "Display/Texture.hpp"
#include "Display/GUI/GUILayout.hpp"
#include "Data/DynamicArray.hpp"
#include "Data/Pointers.hpp"
#include "Display/OpenGLHelper.hpp"

#ifdef OPENGL_ENABLED
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#endif

class OpenGLRenderer : public IRenderer
{
  private:
    struct MatrixBuffer
    {
        glm::mat4 world;
        glm::mat4 view;
        glm::mat4 projection;
    };

    struct VertexInfo
    {
        glm::vec3 position;
        glm::vec4 color;
        glm::vec2 texCoord;
        uint32_t cmode;
        uint32_t gui;
    };

    GLFWwindow *pWindow = nullptr;
    int windowWidth = 800;
    int windowHeight = 600;

    // Shader program
    GLuint shaderProgram = 0;

    // Uniform block binding point
    GLuint uboMatrix = 0;
    GLuint defaultVAO = 0;

    // Render state
    WeakPtr<Camera> pCamera;
    bool cameraChanged = true;

    glm::mat4 viewMatrix{1.0f};
    glm::mat4 projMatrix{1.0f};

    // Buffers for scene renderables
    DynamicArray<GLuint> vertexBuffers;
    DynamicArray<GLuint> indexBuffers;
    DynamicArray<GLuint> vertexArrays;
    DynamicArray<GLuint> textures;
    DynamicArray<GLuint> textureUnits;
    DynamicArray<size_t> indexCounts;

    void CreateDefaultShader();
    bool LoadRenderable(IRenderable &renderable, Scene::SceneType sceneType);
    void RenderRenderable(IRenderable &renderable);
    VertexInfo *CreateInputBuffer(const IRenderable &renderable, Scene::SceneType sceneType);
    void UpdateMatrixUniforms(const Point3D &pos, const Point3D &rot, const Point3D &scale);

  public:
    OpenGLRenderer();
    ~OpenGLRenderer();

    virtual bool Initialize(const Window *pWindow) override;

    virtual bool SwitchToFullScreen() override;
    virtual bool SwitchToWindowMode() override;

    virtual bool LoadScene(Scene &scene) override;
    virtual bool LoadGUILayout(GUILayout &layout) override;

    virtual bool AddVertexShader(Shader &shader) override;
    virtual bool AddPixelShader(Shader &shader) override;

    virtual void ApplyCamera(WeakPtr<Camera> pCamera) override;
    virtual void OnCameraChanged() override;
    virtual void OnWindowResized(long newWidth, long newHeight) override;

    virtual void Update() override;
    virtual void Render(Scene &scene) override;
    virtual void Render(GUILayout &layout) override;
    virtual void Clear() override;

    friend class Shader;
};

#endif