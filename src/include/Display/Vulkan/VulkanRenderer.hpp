#ifndef VULKAN_RENDERER_HPP
#define VULKAN_RENDERER_HPP

#include "Display/IRenderer.hpp"
#include "pch.hpp"

class Window;

class VulkanRenderer : public IRenderer
{
  private:
    VkInstance vulkanInstance;

    VkDebugUtilsMessengerEXT debugMessenger;

    bool CheckSupportedExtensions(DynamicArray<const char *> *pGLFWExtensionNames);

    bool CheckSupportedValidationLayers(const DynamicArray<const char *> *pValidationLayers);

    bool SetupDebugMessenger();

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                        VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                        void *pUserData);

  public:
    VulkanRenderer();

    ~VulkanRenderer();

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
};

#endif
