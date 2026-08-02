#include "Display/Vulkan/VulkanRenderer.hpp"

#include "App/App.hpp"
#include "Data/DynamicArray.hpp"
#include "Display/Vulkan/VulkanHelper.hpp"
#include "Display/Window.hpp"
#include "System/Debug/Debug.hpp"

#include <algorithm>
#include <cstring>

// ══════════════════════════════════════════════════════════════════════════════
//  Surface / Swapchain / Image Views
// ══════════════════════════════════════════════════════════════════════════════

bool VulkanRenderer::CreateSurface(const Window *pWindow)
{
    VkResult res = glfwCreateWindowSurface(vulkanInstance, pWindow->GetHandle(), nullptr, &surface);
    if (res != VK_SUCCESS)
    {
        PRINTLN_ERR(L"VulkanRenderer: failed to create window surface.");
        return false;
    }
    return true;
}

VkSurfaceFormatKHR VulkanRenderer::ChooseSwapSurfaceFormat(
    const DynamicArray<VkSurfaceFormatKHR> &availableFormats) const
{
    for (size_t i = 0; i < availableFormats.Length(); i++)
    {
        if (availableFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return availableFormats[i];
    }
    return availableFormats[0];
}

VkPresentModeKHR VulkanRenderer::ChooseSwapPresentMode(
    const DynamicArray<VkPresentModeKHR> &availablePresentModes) const
{
    for (size_t i = 0; i < availablePresentModes.Length(); i++)
    {
        if (availablePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
            return availablePresentModes[i];
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanRenderer::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &caps) const
{
    if (caps.currentExtent.width != UINT32_MAX)
        return caps.currentExtent;

    int width, height;
    glfwGetFramebufferSize(pWindow->GetHandle(), &width, &height);

    VkExtent2D actualExtent = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)
    };
    actualExtent.width  = std::clamp(actualExtent.width,  caps.minImageExtent.width,  caps.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height, caps.minImageExtent.height, caps.maxImageExtent.height);
    return actualExtent;
}

bool VulkanRenderer::CreateSwapchain()
{
    // ── Query surface capabilities ──────────────────────────────────────
    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &caps);

    // ── Surface formats ─────────────────────────────────────────────────
    uint32_t nFormats = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &nFormats, nullptr);
    DynamicArray<VkSurfaceFormatKHR> formats;
    formats.Resize(nFormats);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &nFormats, &formats[0]);
    VkSurfaceFormatKHR format = ChooseSwapSurfaceFormat(formats);

    // ── Present modes ───────────────────────────────────────────────────
    uint32_t nPresentModes = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &nPresentModes, nullptr);
    DynamicArray<VkPresentModeKHR> presentModes;
    presentModes.Resize(nPresentModes);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &nPresentModes, &presentModes[0]);
    VkPresentModeKHR presentMode = ChooseSwapPresentMode(presentModes);

    // ── Image count ─────────────────────────────────────────────────────
    uint32_t imageCount = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 && imageCount > caps.maxImageCount)
        imageCount = caps.maxImageCount;

    // ── Create info ─────────────────────────────────────────────────────
    VkSwapchainCreateInfoKHR swapchainInfo = {};
    swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainInfo.surface = surface;
    swapchainInfo.minImageCount = imageCount;
    swapchainInfo.imageFormat = format.format;
    swapchainInfo.imageColorSpace = format.colorSpace;
    swapchainInfo.imageExtent = ChooseSwapExtent(caps);
    swapchainInfo.imageArrayLayers = 1;
    // COLOR_ATTACHMENT:未來的 render pass;TRANSFER_DST:目前的清色路徑(vkCmdClearColorImage)
    swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    // Queue families
    QueueFamilyIndices indices;
    indices.graphics = graphicsQueueFamilyIndex;
    indices.present = presentQueueFamilyIndex;
    if (indices.graphics != indices.present)
    {
        uint32_t familyIndices[] = { indices.graphics, indices.present };
        swapchainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchainInfo.queueFamilyIndexCount = 2;
        swapchainInfo.pQueueFamilyIndices = familyIndices;
    }
    else
    {
        swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainInfo.queueFamilyIndexCount = 0;
        swapchainInfo.pQueueFamilyIndices = nullptr;
    }

    swapchainInfo.preTransform = caps.currentTransform;
    swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainInfo.presentMode = presentMode;
    swapchainInfo.clipped = VK_TRUE;
    swapchainInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device, &swapchainInfo, nullptr, &swapchain) != VK_SUCCESS)
    {
        PRINTLN_ERR(L"VulkanRenderer: failed to create swapchain.");
        return false;
    }

    // ── Retrieve swapchain images ───────────────────────────────────────
    swapchainImageFormat = format.format;
    swapchainExtent = swapchainInfo.imageExtent;

    uint32_t nImages = 0;
    vkGetSwapchainImagesKHR(device, swapchain, &nImages, nullptr);
    swapchainImages.Resize(nImages);
    vkGetSwapchainImagesKHR(device, swapchain, &nImages, &swapchainImages[0]);

    return true;
}

bool VulkanRenderer::CreateSwapchainImageViews()
{
    swapchainImageViews.Resize(swapchainImages.Length());
    for (size_t i = 0; i < swapchainImages.Length(); i++)
    {
        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = swapchainImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = swapchainImageFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &viewInfo, nullptr, &swapchainImageViews[i]) != VK_SUCCESS)
        {
            PRINTLN_ERR(L"VulkanRenderer: failed to create swapchain image view.");
            return false;
        }
    }
    return true;
}

void VulkanRenderer::CleanupSwapchain()
{
    if (!device)
        return;  // Initialize failed before device creation; nothing to destroy

    for (size_t i = 0; i < swapchainImageViews.Length(); i++)
        vkDestroyImageView(device, swapchainImageViews[i], nullptr);
    swapchainImageViews.RemoveAll();

    vkDestroySwapchainKHR(device, swapchain, nullptr);
    swapchain = nullptr;

    swapchainImages.RemoveAll();
}

// ══════════════════════════════════════════════════════════════════════════════
//  Command pool / buffers / sync objects
// ══════════════════════════════════════════════════════════════════════════════

bool VulkanRenderer::CreateCommandPool()
{
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = graphicsQueueFamilyIndex;

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
    {
        PRINTLN_ERR(L"VulkanRenderer: failed to create command pool.");
        return false;
    }
    return true;
}

bool VulkanRenderer::CreateCommandBuffers()
{
    commandBuffers.Resize(swapchainImages.Length());

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = commandBuffers.Length();

    if (vkAllocateCommandBuffers(device, &allocInfo, &commandBuffers[0]) != VK_SUCCESS)
    {
        PRINTLN_ERR(L"VulkanRenderer: failed to allocate command buffers.");
        return false;
    }
    return true;
}

bool VulkanRenderer::CreateSyncObjects()
{
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;  // first vkWaitForFences won't block

    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS ||
        vkCreateFence(device, &fenceInfo, nullptr, &inFlightFence) != VK_SUCCESS)
    {
        PRINTLN_ERR(L"VulkanRenderer: failed to create synchronization objects.");
        return false;
    }
    return true;
}

void VulkanRenderer::CleanupSyncObjects()
{
    if (imageAvailableSemaphore)
    {
        vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
        imageAvailableSemaphore = nullptr;
    }
    if (renderFinishedSemaphore)
    {
        vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
        renderFinishedSemaphore = nullptr;
    }
    if (inFlightFence)
    {
        vkDestroyFence(device, inFlightFence, nullptr);
        inFlightFence = nullptr;
    }

    if (commandPool)
    {
        vkDestroyCommandPool(device, commandPool, nullptr);
        commandPool = nullptr;
    }
    commandBuffers.RemoveAll();
}

// ══════════════════════════════════════════════════════════════════════════════
//  Instance helpers
// ══════════════════════════════════════════════════════════════════════════════

bool VulkanRenderer::CheckSupportedExtensions(DynamicArray<const char *> *pGLFWExtensionNames)
{
    uint32_t nExtensionsSupported;
    DynamicArray<VkExtensionProperties> extensionsSupported;
    auto result = VulkanHelper::ListSupportedExtensions(NULL, &nExtensionsSupported, &extensionsSupported);
    if (!result)
        return false;
    for (size_t i = 0; i < pGLFWExtensionNames->Length(); i++)
    {
        bool found = false;
        for (size_t j = 0; j < extensionsSupported.Length(); j++)
        {
            if (!strcmp((*pGLFWExtensionNames)[i], extensionsSupported[j].extensionName))
            {
                found = true;
                break;
            }
        }
        if (!found)
            return false;
    }
    return true;
}

bool VulkanRenderer::CheckSupportedValidationLayers(const DynamicArray<const char *> *pValidationLayers)
{
#ifdef NDEBUG
    return true;
#endif
    uint32_t nLayers;
    DynamicArray<VkLayerProperties> layers;
    if (!VulkanHelper::ListSupportedValidationLayers(&nLayers, &layers))
        return false;

    for (size_t i = 0; i < pValidationLayers->Length(); i++)
    {
        bool found = false;
        for (size_t j = 0; j < layers.Length(); j++)
        {
            if (!strcmp((*pValidationLayers)[i], layers[j].layerName))
            {
                found = true;
                break;
            }
        }
        if (!found)
            return false;
    }
    return true;
}

bool VulkanRenderer::SetupDebugMessenger()
{
#ifdef NDEBUG
    return true;
#endif
    VkDebugUtilsMessengerCreateInfoEXT messengerInfo = {};
    messengerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    messengerInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    messengerInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    messengerInfo.pfnUserCallback = debugCallback;
    messengerInfo.pUserData = nullptr;
    return VulkanHelper::CreateDebugUtilsMessengerEXT(vulkanInstance, &messengerInfo, NULL, &debugMessenger) ==
           VK_SUCCESS;
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanRenderer::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                             VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                             const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                             void *pUserData)
{
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        PRINTLN_ERR(pCallbackData->pMessage);
    }
    return VK_FALSE;
}

// ══════════════════════════════════════════════════════════════════════════════
//  Device helpers
// ══════════════════════════════════════════════════════════════════════════════

bool VulkanRenderer::SelectPhysicalDevice(VkSurfaceKHR surface)
{
    uint32_t nDevices = 0;
    vkEnumeratePhysicalDevices(vulkanInstance, &nDevices, nullptr);
    if (nDevices == 0)
        return false;

    DynamicArray<VkPhysicalDevice> candidates;
    candidates.Resize(nDevices);
    vkEnumeratePhysicalDevices(vulkanInstance, &nDevices, &candidates[0]);

    for (size_t i = 0; i < candidates.Length(); i++)
    {
        QueueFamilyIndices indices;
        if (IsDeviceSuitable(candidates[i], surface, indices))
        {
            physicalDevice = candidates[i];
            graphicsQueueFamilyIndex = indices.graphics;
            presentQueueFamilyIndex = indices.present;
            return true;
        }
    }

    return false;
}

bool VulkanRenderer::IsDeviceSuitable(VkPhysicalDevice candidate, VkSurfaceKHR surface,
                                      QueueFamilyIndices &indices)
{
    // ── Queue family support ───────────────────────────────────────────
    uint32_t nQueueFamilies = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(candidate, &nQueueFamilies, nullptr);
    DynamicArray<VkQueueFamilyProperties> queueFamilies;
    queueFamilies.Resize(nQueueFamilies);
    vkGetPhysicalDeviceQueueFamilyProperties(candidate, &nQueueFamilies, &queueFamilies[0]);

    for (uint32_t i = 0; i < nQueueFamilies; i++)
    {
        // Graphics queue
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            indices.graphics = i;

        // Present queue (must check surface support)
        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(candidate, i, surface, &presentSupport);
        if (presentSupport)
            indices.present = i;
    }

    if (!indices.IsComplete())
        return false;

    // ── Required device extensions ─────────────────────────────────────
    if (!CheckDeviceExtensionSupport(candidate))
        return false;

    // ── Swapchain capabilities (basic sanity) ──────────────────────────
    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(candidate, surface, &caps);
    if (caps.maxImageCount > 0 && caps.minImageCount > caps.maxImageCount)
        return false;

    return true;
}

bool VulkanRenderer::CheckDeviceExtensionSupport(VkPhysicalDevice candidate)
{
    constexpr uint32_t nRequired = sizeof(RequiredDeviceExtensions) / sizeof(RequiredDeviceExtensions[0]);

    uint32_t nExtensions;
    vkEnumerateDeviceExtensionProperties(candidate, nullptr, &nExtensions, nullptr);
    DynamicArray<VkExtensionProperties> available;
    available.Resize(nExtensions);
    vkEnumerateDeviceExtensionProperties(candidate, nullptr, &nExtensions, &available[0]);

    for (uint32_t r = 0; r < nRequired; r++)
    {
        bool found = false;
        for (size_t a = 0; a < available.Length(); a++)
        {
            if (strcmp(RequiredDeviceExtensions[r], available[a].extensionName) == 0)
            {
                found = true;
                break;
            }
        }
        if (!found)
            return false;
    }
    return true;
}

bool VulkanRenderer::CreateLogicalDevice(const QueueFamilyIndices &indices)
{
    // Unique queue families (max 2: graphics + present)
    uint32_t uniqueFamilyIds[2];
    uint32_t nUnique = 1;
    uniqueFamilyIds[0] = indices.graphics;
    if (indices.present != indices.graphics)
    {
        uniqueFamilyIds[1] = indices.present;
        nUnique = 2;
    }

    DynamicArray<VkDeviceQueueCreateInfo> queueCreateInfos;
    float queuePriority = 1.0f;

    for (uint32_t i = 0; i < nUnique; i++)
    {
        VkDeviceQueueCreateInfo qInfo = {};
        qInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        qInfo.queueFamilyIndex = uniqueFamilyIds[i];
        qInfo.queueCount = 1;
        qInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.Append(qInfo);
    }

    // Device features (none beyond baseline for now)
    VkPhysicalDeviceFeatures features = {};

    VkDeviceCreateInfo deviceInfo = {};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.pQueueCreateInfos = &queueCreateInfos[0];
    deviceInfo.queueCreateInfoCount = queueCreateInfos.Length();
    deviceInfo.pEnabledFeatures = &features;
    deviceInfo.enabledExtensionCount = sizeof(RequiredDeviceExtensions) / sizeof(RequiredDeviceExtensions[0]);
    deviceInfo.ppEnabledExtensionNames = RequiredDeviceExtensions;

    // Device-layer validation (deprecated but kept for older drivers)
#ifdef NDEBUG
    deviceInfo.enabledLayerCount = 0;
#else
    const char *validationLayers[] = {"VK_LAYER_KHRONOS_validation"};
    deviceInfo.enabledLayerCount = 1;
    deviceInfo.ppEnabledLayerNames = validationLayers;
#endif

    if (vkCreateDevice(physicalDevice, &deviceInfo, nullptr, &device) != VK_SUCCESS)
        return false;

    vkGetDeviceQueue(device, indices.graphics, 0, &graphicsQueue);
    vkGetDeviceQueue(device, indices.present, 0, &presentQueue);
    return true;
}

// ══════════════════════════════════════════════════════════════════════════════
//  Constructor / Destructor
// ══════════════════════════════════════════════════════════════════════════════

VulkanRenderer::VulkanRenderer()
    : IRenderer(),
      vulkanInstance(),
      debugMessenger(),
      pWindow(nullptr),
      surface(),
      swapchain(),
      swapchainImageFormat(),
      swapchainExtent(),
      swapchainImages(),
      swapchainImageViews(),
      commandPool(),
      commandBuffers(),
      imageAvailableSemaphore(),
      renderFinishedSemaphore(),
      inFlightFence(),
      currentImageIndex(0),
      physicalDevice(),
      device(),
      graphicsQueue(),
      presentQueue(),
      graphicsQueueFamilyIndex(UINT32_MAX),
      presentQueueFamilyIndex(UINT32_MAX),
      pActiveCamera(nullptr)
{
}

VulkanRenderer::~VulkanRenderer()
{
    CleanupSyncObjects();
    CleanupSwapchain();

    if (surface)
    {
        vkDestroySurfaceKHR(vulkanInstance, surface, nullptr);
        surface = nullptr;
    }

    if (device)
    {
        vkDestroyDevice(device, nullptr);
        device = nullptr;
    }
    VulkanHelper::DestroyDebugUtilsMessengerEXT(vulkanInstance, debugMessenger, NULL);
    if (vulkanInstance)
        vkDestroyInstance(vulkanInstance, nullptr);
}

// ══════════════════════════════════════════════════════════════════════════════
//  IRenderer interface
// ══════════════════════════════════════════════════════════════════════════════

bool VulkanRenderer::Initialize(const Window *pWindow)
{
    this->pWindow = pWindow;

    // ── 1. GLFW instance extensions ────────────────────────────────────
    uint32_t nGLFWExtensions = 0;
    DynamicArray<const char *> pRequiredExtensionNames;
    auto pGLFWExtensions = glfwGetRequiredInstanceExtensions(&nGLFWExtensions);
    for (size_t i = 0; i < nGLFWExtensions; i++)
        pRequiredExtensionNames.Append(pGLFWExtensions[i]);

#ifdef __APPLE__
    pRequiredExtensionNames.Append(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif

#ifndef NDEBUG
    pRequiredExtensionNames.Append(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    if (!CheckSupportedExtensions(&pRequiredExtensionNames))
        return false;

    // ── 2. Validation layers ───────────────────────────────────────────
    DynamicArray<const char *> validationLayers;
    uint32_t nValidationLayers = 0;
#ifndef NDEBUG
    validationLayers.Append("VK_LAYER_KHRONOS_validation");
    nValidationLayers = validationLayers.Length();
    if (!CheckSupportedValidationLayers(&validationLayers))
        return false;
#endif

    // ── 3. Create instance ─────────────────────────────────────────────
    char appName[1024];
    const auto &winInfo = pWindow->GetWindowInfo();
    winInfo.pAppInfo->appName.ToUTF8(appName, 1024);
    if (!VulkanHelper::CreateInstance(appName, winInfo.pAppInfo->appVersion, "Chia Engine",
                                      winInfo.pAppInfo->engineVersion,
                                      pRequiredExtensionNames.Length(), &pRequiredExtensionNames[0],
                                      nValidationLayers,
                                      nValidationLayers ? &validationLayers[0] : NULL,
                                      NULL, &vulkanInstance))
        return false;

    if (!SetupDebugMessenger())
        return false;

    // ── 4. Create surface ──────────────────────────────────────────────
    if (!CreateSurface(pWindow))
        return false;

    // ── 5. Select physical device ──────────────────────────────────────
    if (!SelectPhysicalDevice(surface))
    {
        PRINTLN_ERR("VulkanRenderer: no suitable physical device found.");
        return false;
    }

    // ── 6. Create logical device ───────────────────────────────────────
    QueueFamilyIndices indices;
    indices.graphics = graphicsQueueFamilyIndex;
    indices.present = presentQueueFamilyIndex;
    if (!CreateLogicalDevice(indices))
    {
        PRINTLN_ERR("VulkanRenderer: failed to create logical device.");
        return false;
    }

    // ── 7. Create swapchain ────────────────────────────────────────────
    if (!CreateSwapchain())
    {
        PRINTLN_ERR("VulkanRenderer: failed to create swapchain.");
        return false;
    }

    // ── 8. Create swapchain image views ────────────────────────────────
    if (!CreateSwapchainImageViews())
    {
        PRINTLN_ERR("VulkanRenderer: failed to create swapchain image views.");
        return false;
    }

    // ── 9. Create command pool + buffers ───────────────────────────────
    if (!CreateCommandPool())
        return false;
    if (!CreateCommandBuffers())
        return false;

    // ── 10. Create sync objects (semaphores + fence) ───────────────────
    if (!CreateSyncObjects())
        return false;

    Debug::PrintLine(L"VulkanRenderer: initialized successfully.");
    return true;
}

bool VulkanRenderer::SwitchToFullScreen()
{
    return true;
}

bool VulkanRenderer::SwitchToWindowMode()
{
    return true;
}

bool VulkanRenderer::LoadScene(Scene &scene)
{
    return true;
}

bool VulkanRenderer::LoadGUILayout(GUILayout &layout)
{
    return true;
}

bool VulkanRenderer::AddVertexShader(Shader &shader)
{
    return true;
}

bool VulkanRenderer::AddPixelShader(Shader &shader)
{
    return true;
}

void VulkanRenderer::ApplyCamera(WeakPtr<Camera> pCamera)
{
}

void VulkanRenderer::OnCameraChanged()
{
}

void VulkanRenderer::OnWindowResized(long newWidth, long newHeight)
{
    (void)newWidth;
    (void)newHeight;
    // swapchain 依賴 surface 尺寸,resize 後整組重建
    vkDeviceWaitIdle(device);
    CleanupSwapchain();
    CreateSwapchain();
    CreateSwapchainImageViews();
}

void VulkanRenderer::Update()
{
}

void VulkanRenderer::Render(Scene &scene)
{
}

void VulkanRenderer::Render(GUILayout &layout)
{
}

void VulkanRenderer::Clear()
{
}

// ══════════════════════════════════════════════════════════════════════════════
//  IFrameExecutor — Frame 驅動的渲染
// ══════════════════════════════════════════════════════════════════════════════

bool VulkanRenderer::Execute(const Frame &frame)
{
    for (size_t i = 0; i < frame.GetNumCommands(); i++)
    {
        const Frame::CommandData &command = frame.GetCommand(i);
        switch (command.command)
        {
            case Frame::Command::BeginFrame:
                if (!BeginFrame())
                    return false;
                break;
            case Frame::Command::SetCamera:
                pActiveCamera = command.pCamera;
                break;
            case Frame::Command::DrawRenderable:
                // TODO(P2.5): 需要 render pass + pipeline + vertex buffer 上傳
                break;
            case Frame::Command::DrawGUILayout:
                // TODO(P2.5): GUI 走 Frame 命令
                break;
            case Frame::Command::EndFrame:
                if (!EndFrame())
                    return false;
                break;
        }
    }
    return true;
}

bool VulkanRenderer::BeginFrame()
{
    vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &inFlightFence);

    uint32_t imageIndex = 0;
    const VkResult acquireResult = vkAcquireNextImageKHR(
        device, swapchain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR)
        return false; // TODO: 觸發 swapchain 重建
    if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR)
        return false;
    currentImageIndex = imageIndex;

    VkCommandBuffer cmdBuffer = commandBuffers[imageIndex];
    vkResetCommandBuffer(cmdBuffer, 0);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    if (vkBeginCommandBuffer(cmdBuffer, &beginInfo) != VK_SUCCESS)
        return false;

    RecordClearCommands(cmdBuffer, swapchainImageFormat);
    return true;
}

void VulkanRenderer::RecordClearCommands(VkCommandBuffer cmdBuffer, VkFormat format)
{
    (void)format;
    const VkImage swapchainImage = swapchainImages[currentImageIndex];

    // UNDEFINED -> TRANSFER_DST_OPTIMAL:準備接收 clear
    VkImageMemoryBarrier toTransfer = {};
    toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    toTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    toTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toTransfer.image = swapchainImage;
    toTransfer.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    toTransfer.subresourceRange.levelCount = 1;
    toTransfer.subresourceRange.layerCount = 1;
    toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(cmdBuffer,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &toTransfer);

    // 清色:深藍黑(引擎暗色調)
    const VkClearColorValue clearColor = {0.02f, 0.04f, 0.08f, 1.0f};
    VkImageSubresourceRange range = {};
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    range.levelCount = 1;
    range.layerCount = 1;
    vkCmdClearColorImage(cmdBuffer, swapchainImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                         &clearColor, 1, &range);

    // TRANSFER_DST_OPTIMAL -> PRESENT_SRC_KHR:準備呈現
    VkImageMemoryBarrier toPresent = {};
    toPresent.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    toPresent.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    toPresent.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    toPresent.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toPresent.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toPresent.image = swapchainImage;
    toPresent.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    toPresent.subresourceRange.levelCount = 1;
    toPresent.subresourceRange.layerCount = 1;
    toPresent.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(cmdBuffer,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &toPresent);
}

bool VulkanRenderer::EndFrame()
{
    VkCommandBuffer cmdBuffer = commandBuffers[currentImageIndex];
    if (vkEndCommandBuffer(cmdBuffer) != VK_SUCCESS)
        return false;

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphore};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence) != VK_SUCCESS)
        return false;

    VkSwapchainKHR swapchains[] = {swapchain};
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &currentImageIndex;

    const VkResult presentResult = vkQueuePresentKHR(presentQueue, &presentInfo);
    return presentResult == VK_SUCCESS || presentResult == VK_SUBOPTIMAL_KHR;
}
