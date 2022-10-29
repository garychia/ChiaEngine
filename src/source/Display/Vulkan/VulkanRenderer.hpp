#ifndef VULKAN_RENDERER_HPP
#define VULKAN_RENDERER_HPP

#include "Display/IRenderer.hpp"
#include "Display/Window.hpp"
#include "pch.hpp"

class VulkanRenderer : public IRenderer
{
  private:
    VkInstance vulkanInstance;

  public:
    VulkanRenderer();

    ~VulkanRenderer();

    virtual bool Initialize(Window *pWindow) override;

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
};

#include "VulkanHelper.hpp"

VulkanRenderer::VulkanRenderer() : IRenderer(), vulkanInstance()
{
}

VulkanRenderer::~VulkanRenderer()
{
}

bool VulkanRenderer::Initialize(Window *pWindow)
{
    uint32_t nExtensions = 0;
    const char **glfwExtensionNames;
    glfwExtensionNames = glfwGetRequiredInstanceExtensions(&nExtensions);

    char appName[1024];
    const auto &winInfo = pWindow->GetWindowInfo();
    winInfo.pAppInfo->appName.ToUTF8(appName, 1024);
    VulkanHelper::CreateInstance(appName, winInfo.pAppInfo->appVersion, "Chia Engine", winInfo.pAppInfo->engineVersion,
                                 nExtensions, glfwExtensionNames, 0, NULL, NULL, &vulkanInstance);
}

#endif
