#ifndef VULKAN_RENDERER_HPP
#define VULKAN_RENDERER_HPP

#include "Display/IRenderer.hpp"
#include "Display/IFrameExecutor.hpp"
#include "Display/FrameInterpreter.hpp" // #84:純解譯器 seam(transform stack / material / camera 狀態)
#include "Display/RendererMath.hpp"     // #84:單一矩陣慣例模組
#include "Display/Color.hpp"
#include "Display/Text/GlyphAtlas.hpp"
#include "Data/HashTable.hpp"
#include "pch.hpp"

#include <glm/glm.hpp>

#include <string>

struct Texture; // forward decl:MaterialSource 以指標持有(RenderInfo 鏈才給完整定義)

class Window;

class VulkanRenderer : public IRenderer, public IFrameExecutor
{
  private:
    // #84:Frame → GPU 的純解譯器。Execute 透過它攤平命令銜(transform stack 64
    // 上限、empty-pop、identity fallback、material/camera 狀態都在 interpreter 內),
    // 本類只做逐 op 的 Vulkan 翻譯。headless 契約測試打這層(renderercontracttest)。
    FrameInterpreter interpreter;
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
    DynamicArray<VkSemaphore> imageAvailableSemaphores;  // 每 frame-in-flight 一組
    DynamicArray<VkSemaphore> renderFinishedSemaphores; // 每 swapchain image 一組
    DynamicArray<VkFence> inFlightFences;                // 每 frame-in-flight 一 fence
    uint32_t currentImageIndex;    // 本幀正在使用的 swapchain image
    uint32_t currentFrameIndex;    // 本幀使用的 frame-in-flight slot(MaxFramesInFlight 輪轉)

    // Device
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    uint32_t graphicsQueueFamilyIndex;
    uint32_t presentQueueFamilyIndex;

    // Frame 狀態
    Camera *pActiveCamera;

    // ── Render pass / pipeline / framebuffers ───────────────────────────────
    VkRenderPass renderPass;
    DynamicArray<VkFramebuffer> swapchainFramebuffers;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    VkShaderModule vertShaderModule;
    VkShaderModule fragShaderModule;

    // ── Descriptors / UBO ───────────────────────────────────────────────────
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorPool descriptorPool;
    VkDescriptorSet descriptorSet;
    VkBuffer uniformBuffer;
    VkDeviceMemory uniformBufferMemory;

    // ── 材質 / 貼圖(單一 slot,v1)────────────────────────────────────────────
        VkImage textureImage;
        VkDeviceMemory textureImageMemory;
        VkImageView textureImageView;
        VkSampler textureSampler;
        bool textureReady;
    std::string loadedTexturePath; // 目前載入的真實貼圖路徑(空 = fallback 白色)

    // ── Depth buffer(依附 swapchain extent,resize 時重建)────────────────────
    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;

        // ── Renderable GPU 資料快取(以 identifier 為 key)─────────────────────────
        struct RenderableGpuData
        {
            VkBuffer vertexBuffer;
            VkDeviceMemory vertexMemory;
            uint32_t vertexCount;
        };

        HashTable<size_t, RenderableGpuData> renderableGpuMap;

    // ── P7d:Frame 新命令狀態───────────────────────────────────────────────
    // #84:transform stack / currentMaterialId / boundMaterialId 全部移入
    // FrameInterpreter(interpreter 成員)。本類不再維護 Flatten 純狀態。

    struct GpuMesh
    {
        VkBuffer vertexBuffer;
        VkDeviceMemory vertexMemory;
        uint32_t vertexCount;
    };
    HashTable<uint64_t, GpuMesh> meshCache; // 以 meshId(content-hash)為 key 的幾何快取

    // ── #55:per-material descriptor 綁定───────────────────────────────
    // MaterialSource 供 View 層命名(定義在 public 區)。
    // MaterialGpuData 是純內部型別,需在 materialCache 成員前完整定義(private)。
    struct MaterialGpuData
    {
        VkImage textureImage;
        VkDeviceMemory textureMemory;
        VkImageView textureImageView;
        VkDescriptorSet descriptorSet; // UBO(binding 0) + 各自 texture(binding 1)
    };
    static constexpr uint32_t MaxMaterials = 16;                // descriptor pool 預留額度
    HashTable<uint64_t, MaterialGpuData> materialCache;        // key = materialId(content-hash)
    // #84:boundMaterialId 移入 FrameInterpreter(Execute 以 op.materialId 傳入)。

    // ── P7e:文字渲染───────────────────────────────────────────────
    struct FontAtlasGpuData
    {
        VkImage image;
        VkDeviceMemory memory;
        VkImageView imageView;
        VkSampler sampler;
    };
    HashTable<uint64_t, FontAtlasGpuData> fontAtlasCache; // 以 fontId(content-hash)為 key 的字型圖集
    VkDescriptorSet textDescriptorSet;                    // 文字用的描述子集(UBO + 字型圖集 sampler)
    VkSampler textSampler;                                // 字型圖集共用的 nearest sampler
    uint64_t boundTextFontId = 0;                       // 目前綁進 textDescriptorSet 的 fontId
    VkBuffer textVertexBuffer;
    VkDeviceMemory textVertexMemory;
    uint32_t textVertexCapacity = 0;

    // #66:文字 vertex buffer 固定容量(≈682 字形/單一 label)。
    // 執行期擴容會 destroy 仍被 in-flight command buffer 參照的舊 buffer
    // (VUID 錯誤);TextLayout 已依佈局寬度截斷 label,固定容量綽綽有餘。
    static constexpr uint32_t MaxTextVertices = 4096;

    // #67:renderableGpuMap 的 key 是 IRenderable::identifier,但 MarkLoaded
    // 從無呼叫者 → 全部 renderable id=0 → Insert 無條件覆寫,後載入的幾何
    // 蓋掉先載入的(cube 被 toolbar 蓋掉 / 所有 GUI rect 共用第一顆資料)。
    // LoadRenderable 首次見到 id==0 時發給唯一 id 並寫回 renderable。
    size_t nextRenderableId = 0;

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
    void RecreateSwapchain(); // #56: resize(#41) 與 acquire 失敗路徑共用

    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const DynamicArray<VkSurfaceFormatKHR> &availableFormats) const;
    VkPresentModeKHR ChooseSwapPresentMode(const DynamicArray<VkPresentModeKHR> &availablePresentModes) const;
    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &caps) const;

    // ── Command / sync helpers ─────────────────────────────────────────────
    bool CreateCommandPool();
    bool CreateCommandBuffers();
    bool CreateSyncObjects();
    void CleanupSyncObjects();

    // ── Frame 執行 ─────────────────────────────────────────────────────────
    bool BeginFrame();                             // acquire + 清畫面命令
    void RecordClearCommands(VkCommandBuffer cmdBuffer, VkFormat format); // 清色 + 轉換 layout
    bool EndFrame();                               // submit + present

    // ── Pipeline 建置 ───────────────────────────────────────────────────────
    bool CreateRenderPass();
    bool CreateFramebuffers();
    void CleanupFramebuffers();
    bool CreateDepthResources();
    void CleanupDepthResources();
    bool CreateDescriptorSetLayout();
    bool CreateDescriptorPool();
    bool CreateGraphicsPipeline();
    bool CreateShaderModule(const unsigned char *pCode, size_t codeSize, VkShaderModule &outModule);
    bool CreateUniformBuffer();
    bool CreateWhiteTexture();
    VkImageView CreateImageView(VkImage image, VkFormat format);
    void CleanupPipelineResources();
    void CleanupRenderableGpuMap();
    void CleanupMeshCache();
    void CleanupMaterialCache();

    // ── Renderable 繪製 ─────────────────────────────────────────────────────
    bool LoadRenderable(const IRenderable &renderable);       // RenderInfo → GPU buffer
    bool LoadTextureImage(const Texture &texture);            // 載入真實貼圖(單一 slot)
    void UpdateTextureDescriptor();                           // 重新寫 UBO + sampler 描述子
    void RecordDrawCommands(VkCommandBuffer cmdBuffer, const IRenderable &renderable, bool guiSpace = false);
    void RecordMeshDrawCommands(VkCommandBuffer cmdBuffer, uint64_t meshId, const glm::mat4 &world,
                                uint64_t materialId); // materialId = interpreter 解析的當前綁定
    void UpdateUniformBuffer(const glm::mat4 &world, const glm::mat4 &view,
                             const glm::mat4 &projection, bool useTexture);

    // ── P7e:文字渲染 helpers ─────────────────────────────────────────
    bool CreateTextSampler();
    void UpdateTextDescriptor(const VkImageView &imageView, VkSampler sampler);
    bool CreateFontAtlasTexture(uint64_t fontId, const GlyphAtlas &atlas);
    void RecordTextDrawCommands(VkCommandBuffer cmdBuffer, uint64_t fontId, const char16_t *text,
                                float size, const Color &color, const glm::mat4 &world);
    void CleanupFontAtlasCache();

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

    // IFrameExecutor
    virtual bool Execute(const Frame &frame) override;

    // ── P7d:mesh 幾何註冊(由 View 側把 renderable 幾何登記成 content-hash meshId)──
    bool RegisterMeshGeometry(uint64_t meshId, const RenderInfo &info);

    // ── #55:material 註冊(材質 id → texture + descriptor set)──────────────
    struct MaterialSource
    {
        const Texture *pTexture = nullptr;        // 非 null → stbi 載入 imagePath
        const unsigned char *pRawRGBA = nullptr;  // 非 null → inline raw RGBA(仿 FontAtlas)
        uint32_t width = 0, height = 0;            // pRawRGBA 用
    };
    bool RegisterMaterial(uint64_t materialId, const MaterialSource &source);
};

#endif
