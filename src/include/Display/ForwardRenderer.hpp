#ifndef FORWARD_RENDERER_HPP
#define FORWARD_RENDERER_HPP

#include <cstdint>

#include "Display/Camera.hpp"
#include "Display/IRenderer.hpp"
#include "Scene/LightComponent.hpp"
#include "Scene/MaterialComponent.hpp"
#include "Scene/SceneSystem.hpp"
#include "Geometry/3D/Point3D.hpp"

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GLFW/glfw3.h>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

// ForwardRenderer:基於 PBR 金屬度/粗糙度工作流的前向渲染器。
// 獨立於現有 Renderer 類別，為 A2 基礎渲染管線提供可測試的渲染 seam。
//
// 渲染流程:
//  1. Initialize():建立 OpenGL 程式(頂點+片段著色器)、VAO、uniform 位置快取
//  2. DrawScene(SceneSystem&):遞迴遍歷場景節點,為每個有 TransformComponent 的節點
//     繪製 PBR 光照(方向光 + 點光)
class ForwardRenderer
{
  public:
    ForwardRenderer();
    ~ForwardRenderer();

    // 初始化 OpenGL 資源(著色器程式、VAO)。需要當前線程有當前 OpenGL 上下文。
    // 回傳 true 表示著色器編譯連結成功。
    bool Initialize();

    // 繪製整個場景(遞迴遍歷根節點及其子樹)。
    void DrawScene(SceneSystem &sceneSystem);

    // 釋放 OpenGL 資源。
    void Shutdown();

    // 著色器程式 ID(0 表示未初始化或失敗)。
    GLuint GetProgram() const { return mProgram; }

    // 著色器是否已成功初始化。
    bool IsInitialized() const { return mInitialized; }

  private:
    // 編譯單一著色器，失敗時回傳 0。
    GLuint CompileShader(GLenum type, const char *source);

    // 建立頂點+片段著色器程式，失敗時回傳 0。
    GLuint CreateProgram(const char *vertSrc, const char *fragSrc);

    // 為單一節點繪製 PBR 光照。
    void DrawNode(SceneSystem &sceneSystem, Entity entity,
                  const glm::mat4 &parentWorld, const Camera &camera);

    GLuint mProgram = 0;
    GLuint mVAO = 0;
    bool mInitialized = false;

    // Uniform 位置快取(-1 表示未找到或著色器未使用)。
    GLint mLocModel = -1;
    GLint mLocView = -1;
    GLint mLocProj = -1;
    GLint mLocAlbedo = -1;
    GLint mLocMetallic = -1;
    GLint mLocRoughness = -1;
    GLint mLocLightDir = -1;
    GLint mLocLightColor = -1;
    GLint mLocLightIntensity = -1;
    GLint mLocLightType = -1;
    GLint mLocViewPos = -1;
};

#endif // FORWARD_RENDERER_HPP
