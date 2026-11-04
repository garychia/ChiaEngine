#ifndef SHADOW_MAPPER_TEST_HPP
#define SHADOW_MAPPER_TEST_HPP

#include "Display/ShadowMapper.hpp"
#include "Scene/LightComponent.hpp"
#include "Scene/SceneSystem.hpp"
#include "Test.hpp"

#include <cstdio>

namespace shadowmapper_test
{

// ShadowMapperTest:headless 可測的 ShadowMapper 契約。
// 1. 可建構(不需 OpenGL 上下文，IsInitialized()=false)
// 2. SceneSystem::GetShadowCasters 正確篩選 Directional + castsShadow=true
// 3. 若可建立 GLFW 無頭視窗 + OpenGL 上下文:Initialize 成功、FBO/深度貼圖 != 0
class ShadowMapperTest : public Test
{
  public:
    ShadowMapperTest() : Test("ShadowMapper")
    {
    }

    bool Run() noexcept override
    {
        TEST_MESSAGE(GetName());

        // ---- AC1:可建構(無 OpenGL 上下文) ----
        {
            ShadowMapper mapper;
            EXPECT_TRUE(!mapper.IsInitialized(), "未 Initialize → IsInitialized()=false", true);
            EXPECT_TRUE(mapper.GetFBO() == 0, "未 Initialize → GetFBO()==0", true);
            EXPECT_TRUE(mapper.GetDepthTexture() == 0,
                        "未 Initialize → GetDepthTexture()==0", true);
        }

        // ---- AC2:SceneSystem::GetShadowCasters 篩選正確 ----
        {
            SceneSystem system;

            // 無光源 → 空
            EXPECT_TRUE(system.GetShadowCasters().GetNElements() == 0,
                        "無光源 → 0 個投射者", true);

            // Point light(即使 castsShadow=true) 不應被選入
            {
                Entity pt = system.CreateNode();
                LightComponent ptLight;
                ptLight.type = LightComponent::Point;
                ptLight.castsShadow = true;
                system.SetLight(pt.GetIndex(), ptLight);
                EXPECT_TRUE(system.GetShadowCasters().GetNElements() == 0,
                            "Point light 不計入投射者", true);
            }

            // Directional light 但 castsShadow=false 不應被選入
            {
                Entity dl1 = system.CreateNode();
                LightComponent dl1Light;
                dl1Light.type = LightComponent::Directional;
                dl1Light.castsShadow = false;
                system.SetLight(dl1.GetIndex(), dl1Light);
                EXPECT_TRUE(system.GetShadowCasters().GetNElements() == 0,
                            "Directional 但 castsShadow=false 不計入", true);
            }

            // Directional + castsShadow=true 應被選入
            {
                Entity dl2 = system.CreateNode();
                LightComponent dl2Light;
                dl2Light.type = LightComponent::Directional;
                dl2Light.castsShadow = true;
                system.SetLight(dl2.GetIndex(), dl2Light);
                DynamicArray<Entity> casters = system.GetShadowCasters();
                EXPECT_TRUE(casters.GetNElements() == 1,
                            "1 個 Directional+castsShadow 投射者", true);
            }

            // 多個 Directional + castsShadow=true → 全部選入
            {
                Entity dl3 = system.CreateNode();
                LightComponent dl3Light;
                dl3Light.type = LightComponent::Directional;
                dl3Light.castsShadow = true;
                system.SetLight(dl3.GetIndex(), dl3Light);
                DynamicArray<Entity> casters = system.GetShadowCasters();
                EXPECT_TRUE(casters.GetNElements() == 2,
                            "2 個 Directional+castsShadow 投射者", true);
            }
        }

        // ---- AC3:若可建立 GLFW 無頭視窗 + OpenGL 上下文，Initialize 成功 ----
        {
            if (!glfwInit())
            {
                TEST_MESSAGE("GLFW 初始化失敗 — 跳過 GPU 陰影貼圖驗證");
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
                TEST_MESSAGE("GLFW 建立視窗失敗 — 跳過 GPU 陰影貼圖驗證");
                EXPECT_TRUE(true, "GLFW 建立視窗失敗 — 跳過 GPU 驗證", true);
                return true;
            }

            glfwMakeContextCurrent(window);

            ShadowMapper mapper;
            bool initOk = mapper.Initialize(1024, 1024);
            EXPECT_TRUE(initOk, "ShadowMapper::Initialize() 成功(OpenGL 3.3 Core)", true);
            if (initOk)
            {
                EXPECT_TRUE(mapper.IsInitialized(), "Initialize 後 IsInitialized()=true", true);
                EXPECT_TRUE(mapper.GetFBO() != 0, "FBO ID != 0", true);
                EXPECT_TRUE(mapper.GetDepthTexture() != 0,
                            "深度貼圖 ID != 0", true);
                EXPECT_TRUE(mapper.GetWidth() == 1024 && mapper.GetHeight() == 1024,
                            "解析度 = 1024x1024", true);

                mapper.BindForReading(1);
                EXPECT_TRUE(true, "BindForReading(1) 不崩潰", true);

                mapper.Shutdown();
                EXPECT_TRUE(!mapper.IsInitialized(), "Shutdown 後 IsInitialized()=false", true);
                EXPECT_TRUE(mapper.GetFBO() == 0, "Shutdown 後 FBO=0", true);
                EXPECT_TRUE(mapper.GetDepthTexture() == 0,
                            "Shutdown 後深度貼圖=0", true);
            }

            glfwDestroyWindow(window);
            glfwTerminate();
        }

        SUCCESS_MESSAGE(GetName());
        return true;
    }

  private:
};

} // namespace shadowmapper_test

#endif // SHADOW_MAPPER_TEST_HPP
