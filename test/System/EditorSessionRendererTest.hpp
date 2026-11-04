#ifndef EDITOR_SESSION_RENDERER_TEST_HPP
#define EDITOR_SESSION_RENDERER_TEST_HPP

#include "Display/GUI/EditorSession.hpp"
#include "Scene/SceneSystem.hpp"
#include "Test.hpp"

#include <cstdio>

#if !defined(VULKAN_ENABLED) && !defined(DIRECTX_ENABLED)
#include "Display/ForwardRenderer.hpp"
#include "Display/ShadowMapper.hpp"
#endif

namespace editorsessionrenderer_test
{

// EditorSessionRendererTest:EditorSession 與 ForwardRenderer/ShadowMapper 串接契約。
// 1. InitializeRenderer 無 OpenGL 上下文時不崩潰(Renderer 可建構但未初始化)
// 2. RenderFrame 未初始化時 no-op
// 3. 有 OpenGL 上下文時 InitializeRenderer 成功、RenderFrame 執行不崩潰
class EditorSessionRendererTest : public Test
{
  public:
    EditorSessionRendererTest() : Test("EditorSessionRenderer")
    {
    }

    bool Run() noexcept override
    {
        TEST_MESSAGE(GetName());

        // ---- AC1:InitializeRenderer 無 OpenGL 上下文時不崩潰 ----
        {
            SceneSystem sceneSystem;
            EditorSession session(&sceneSystem, nullptr, nullptr, nullptr, nullptr);

            // 未建立 OpenGL 上下文 → InitializeRenderer 會建立 ForwardRenderer，
            // 但 ForwardRenderer::Initialize() 需要當前 OpenGL 上下文，
            // 在無頭環境下 glfwInit/glfwCreateWindow 可能失敗 → 回傳 false，不崩潰。
            bool initOk = session.InitializeRenderer();
            (void)initOk; // 取決於環境是否有 OpenGL 上下文
            EXPECT_TRUE(true, "InitializeRenderer 不崩潰(無 OpenGL 上下文)", true);
        }

        // ---- AC2:RenderFrame 未初始化時 no-op ----
        {
            SceneSystem sceneSystem;
            EditorSession session(&sceneSystem, nullptr, nullptr, nullptr, nullptr);

            // 未 InitializeRenderer → RenderFrame 應為 no-op，不崩潰
            session.RenderFrame();
            EXPECT_TRUE(true, "RenderFrame 未初始化時 no-op", true);
        }

        // ---- AC3:IsRendererInitialized 初始為 false ----
        {
            SceneSystem sceneSystem;
            EditorSession session(&sceneSystem, nullptr, nullptr, nullptr, nullptr);
            EXPECT_TRUE(!session.IsRendererInitialized(),
                        "未 InitializeRenderer → IsRendererInitialized()=false", true);
        }

        // ---- AC4:若可建立 GLFW 無頭視窗 + OpenGL 上下文，完整流程 ----
        {
            if (!glfwInit())
            {
                TEST_MESSAGE("GLFW 初始化失敗 — 跳過 GPU 渲染器串接驗證");
                EXPECT_TRUE(true, "GLFW 不可用 — 跳過 GPU 驗證", true);
                return true;
            }

            glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

            GLFWwindow *window = glfwCreateWindow(1, 1, "headless", nullptr, nullptr);
            if (!window)
            {
                glfwTerminate();
                TEST_MESSAGE("GLFW 建立視窗失敗 — 跳過 GPU 渲染器串接驗證");
                EXPECT_TRUE(true, "GLFW 建立視窗失敗 — 跳過 GPU 驗證", true);
                return true;
            }

            glfwMakeContextCurrent(window);

            SceneSystem sceneSystem4;
            Entity root = sceneSystem4.CreateNode();

            EditorSession session4(&sceneSystem4, nullptr, nullptr, nullptr, nullptr);
            bool initOk = session4.InitializeRenderer();
            EXPECT_TRUE(initOk, "EditorSession::InitializeRenderer() 成功(OpenGL 3.3 Core)", true);

            if (initOk)
            {
                EXPECT_TRUE(session4.IsRendererInitialized(),
                            "InitializeRenderer 後 IsRendererInitialized()=true", true);

                // RenderFrame 應能執行不崩潰
                session4.RenderFrame();
                EXPECT_TRUE(true, "RenderFrame 執行不崩潰", true);

                session4.ShutdownRenderer();
                EXPECT_TRUE(!session4.IsRendererInitialized(),
                            "ShutdownRenderer 後 IsRendererInitialized()=false", true);
            }

            glfwDestroyWindow(window);
            glfwTerminate();
        }

        SUCCESS_MESSAGE(GetName());
        return true;
    }

  private:
};

} // namespace editorsessionrenderer_test

#endif // EDITOR_SESSION_RENDERER_TEST_HPP
