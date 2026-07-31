#ifndef VULKAN_RENDERER_HPP
#define VULKAN_RENDERER_HPP

#include "Display/IRenderer.hpp"
#include "pch.hpp"

class Window;

class VulkanRenderer : public IRenderer
{
  private:
    // Instance
    VkInstance vulkanInstance;
    VkDebugUtilsMessengerEXT debugMessenger;

    // Window (for swapchain extent queries)
    const Window *pWindow;

    // Surface / Swapchain
    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;
    VkFormat swapchainImageFormat;
    VkExtent2D swapchainExtent;
    DynamicArray<VkImage> swapchainImages;
    DynamicArray<VkImageView> swapchainImageViews;

    // Command pool / buffers / sync
    VkCommandPool commandPool;
    DynamicArray<VkCommandBuffer> commandBuffers;
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence inFlightFence;

    // Device
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    uint32_t graphicsQueueFamilyIndex;
    uint32_t presentQueueFamilyIndex;

    // ── Instance helpers ──────────────────────────────────────────────────
    bool CheckSupportedExtensions(DynamicArray<const char *> *pGLFWExtensionNames);
    bool CheckSupportedValidationLayers(const DynamicArray<const char *> *pValidationLayers);
    bool SetupDebugMessenger();
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
        void *pUserData);

    // ── Surface / Swapchain helpers ───────────────────────────────────────
    bool CreateSurface(const Window *pWindow);
    bool CreateSwapchain();
    bool CreateSwapchainImageViews();
    void CleanupSwapchain();

    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const DynamicArray<VkSurfaceFormatKHR> &availableFormats) const;
    VkPresentModeKHR ChooseSwapPresentMode(const DynamicArray<VkPresentModeKHR> &availablePresentModes) const;
    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &caps) const;

    // ── Command / sync helpers ─────────────────────────────────────────────
    bool CreateCommandPool();
    bool CreateCommandBuffers();
    bool CreateSyncObjects();
    void CleanupSyncObjects();

    // ── Device helpers ────────────────────────────────────────────────────
    struct QueueFamilyIndices
    {
        uint32_t graphics = UINT32_MAX;
        uint32_t present  = UINT32_MAX;
        bool IsComplete() const { return graphics != UINT32_MAX && present != UINT32_MAX; }
    };

    bool SelectPhysicalDevice(VkSurfaceKHR surface);
    bool IsDeviceSuitable(VkPhysicalDevice candidate, VkSurfaceKHR surface, QueueFamilyIndices &indices);
    bool CheckDeviceExtensionSupport(VkPhysicalDevice candidate);

    bool CreateLogicalDevice(const QueueFamilyIndices &indices);

    // ── Helpers ───────────────────────────────────────────────────────────
    static constexpr const char *RequiredDeviceExtensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

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
