#include "Display/Vulkan/VulkanRenderer.hpp"

#include "App/App.hpp"
#include "Data/DynamicArray.hpp"
#include "Display/Vulkan/VulkanHelper.hpp"
#include "Display/Vulkan/shaders/default_vert_spv.h"
#include "Display/Vulkan/shaders/default_frag_spv.h"
#include "Display/Window.hpp"
#include "Display/Texture.hpp"
#include "Display/IRenderable.hpp"
#include "Display/GUI/GUILayer.hpp"
#include "System/Debug/Debug.hpp"

#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "3rdparty/stb_image.h"

#include <algorithm>
#include <cmath>
#include <cstring>

// ══════════════════════════════════════════════════════════════════════════════
//  Vertex / UBO 格式(與 shader 及 OpenGL 版 CreateInputBuffer 對應)
// ══════════════════════════════════════════════════════════════════════════════

struct VulkanVertex
{
    glm::vec4 position; // w = 1
    glm::vec4 color;
    glm::vec2 texCoord;
    uint32_t cmode;
    uint32_t gui;
};

struct MatrixBuffer
{
    glm::mat4 world;
    glm::mat4 view;
    glm::mat4 projection;
    float useTexture;
};

// ══════════════════════════════════════════════════════════════════════════════
//  矩陣慣例與 OpenGLHelper 一致(該檔只在 OPENGL build 編譯,故在此複製)
// ══════════════════════════════════════════════════════════════════════════════

static glm::mat4 BuildViewMatrix(const Point3D &pos, const Point3D &rot)
{
    glm::vec3 position(pos.x, pos.y, pos.z);
    glm::vec3 rotation(rot.x, rot.y, rot.z);
    glm::vec3 front;
    front.x = cos(glm::radians(rotation.y)) * cos(glm::radians(rotation.x));
    front.y = sin(glm::radians(rotation.x));
    front.z = sin(glm::radians(rotation.y)) * cos(glm::radians(rotation.x));
    return glm::lookAt(position, position + glm::normalize(front), glm::vec3(0.0f, 1.0f, 0.0f));
}

static glm::mat4 BuildProjMatrix(float fovDeg, float aspect, float nearP, float farP)
{
    return glm::perspective(glm::radians(fovDeg), aspect, nearP, farP);
}

static glm::mat4 BuildWorldMatrix(const Point3D &pos, const Point3D &rot, const Point3D &scale)
{
    glm::mat4 world(1.0f);
    world = glm::translate(world, glm::vec3(pos.x, pos.y, pos.z));
    world = glm::rotate(world, glm::radians(rot.x), glm::vec3(1.0f, 0.0f, 0.0f));
    world = glm::rotate(world, glm::radians(rot.y), glm::vec3(0.0f, 1.0f, 0.0f));
    world = glm::rotate(world, glm::radians(rot.z), glm::vec3(0.0f, 0.0f, 1.0f));
    world = glm::scale(world, glm::vec3(scale.x, scale.y, scale.z));
    return world;
}

// ══════════════════════════════════════════════════════════════════════════════
//  記憶體 / buffer / 貼圖上傳小工具
// ══════════════════════════════════════════════════════════════════════════════

static uint32_t FindMemoryType(VkPhysicalDevice physDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physDevice, &memProperties);
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1u << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    }
    return UINT32_MAX;
}

static bool CreateBuffer(VkDevice device, VkPhysicalDevice physDevice, VkDeviceSize size,
                         VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                         VkBuffer &outBuffer, VkDeviceMemory &outMemory)
{
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(device, &bufferInfo, nullptr, &outBuffer) != VK_SUCCESS)
        return false;

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, outBuffer, &memRequirements);
    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(physDevice, memRequirements.memoryTypeBits, properties);
    if (allocInfo.memoryTypeIndex == UINT32_MAX)
        return false;
    if (vkAllocateMemory(device, &allocInfo, nullptr, &outMemory) != VK_SUCCESS)
        return false;
    vkBindBufferMemory(device, outBuffer, outMemory, 0);
    return true;
}

static bool SubmitSingleTimeCommands(VkDevice device, VkQueue queue, VkCommandPool pool, VkCommandBuffer cmdBuffer)
{
    if (vkEndCommandBuffer(cmdBuffer) != VK_SUCCESS)
        return false;
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuffer;
    if (vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
        return false;
    vkQueueWaitIdle(queue);
    vkFreeCommandBuffers(device, pool, 1, &cmdBuffer);
    return true;
}

// 建立 2D RGBA8 貼圖(staging buffer → device local),並轉到 SHADER_READ_ONLY
static bool CreateImageWithData(VkDevice device, VkPhysicalDevice physDevice, VkQueue queue, VkCommandPool pool,
                                uint32_t width, uint32_t height, const void *pPixels,
                                VkImage &outImage, VkDeviceMemory &outMemory)
{
    const VkDeviceSize imageSize = static_cast<VkDeviceSize>(width) * height * 4;

    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
    if (!CreateBuffer(device, physDevice, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                      stagingBuffer, stagingMemory))
        return false;
    void *pData = nullptr;
    vkMapMemory(device, stagingMemory, 0, imageSize, 0, &pData);
    memcpy(pData, pPixels, imageSize);
    vkUnmapMemory(device, stagingMemory);

    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateImage(device, &imageInfo, nullptr, &outImage) != VK_SUCCESS)
    {
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingMemory, nullptr);
        return false;
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, outImage, &memRequirements);
    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(physDevice, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (vkAllocateMemory(device, &allocInfo, nullptr, &outMemory) != VK_SUCCESS)
    {
        vkDestroyImage(device, outImage, nullptr);
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingMemory, nullptr);
        return false;
    }
    vkBindImageMemory(device, outImage, outMemory, 0);

    VkCommandBufferAllocateInfo allocCmd = {};
    allocCmd.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocCmd.commandPool = pool;
    allocCmd.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocCmd.commandBufferCount = 1;
    VkCommandBuffer cmdBuffer = VK_NULL_HANDLE;
    vkAllocateCommandBuffers(device, &allocCmd, &cmdBuffer);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmdBuffer, &beginInfo);

    VkImageMemoryBarrier toTransfer = {};
    toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    toTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    toTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toTransfer.image = outImage;
    toTransfer.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    toTransfer.subresourceRange.levelCount = 1;
    toTransfer.subresourceRange.layerCount = 1;
    toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &toTransfer);

    VkBufferImageCopy region = {};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent.width = width;
    region.imageExtent.height = height;
    region.imageExtent.depth = 1;
    vkCmdCopyBufferToImage(cmdBuffer, stagingBuffer, outImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    VkImageMemoryBarrier toShader = toTransfer;
    toShader.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    toShader.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    toShader.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    toShader.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &toShader);

    SubmitSingleTimeCommands(device, queue, pool, cmdBuffer);
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingMemory, nullptr);
    return true;
}

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
    swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

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

    // 每 swapchain image 一組 semaphore:semaphore 重用若跨越「present 尚未
    // 重新 acquire」的 image,validation 每幀報警(見 swapchain_semaphore_reuse)
    imageAvailableSemaphores.RemoveAll();
    renderFinishedSemaphores.RemoveAll();
    imageAvailableSemaphores.Resize(swapchainImages.Length());
    renderFinishedSemaphores.Resize(swapchainImages.Length());
    for (size_t i = 0; i < swapchainImages.Length(); i++)
    {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS)
        {
            PRINTLN_ERR(L"VulkanRenderer: failed to create synchronization objects.");
            return false;
        }
    }
    if (vkCreateFence(device, &fenceInfo, nullptr, &inFlightFence) != VK_SUCCESS)
    {
        PRINTLN_ERR(L"VulkanRenderer: failed to create synchronization objects.");
        return false;
    }
    return true;
}

void VulkanRenderer::CleanupSyncObjects()
{
    for (size_t i = 0; i < imageAvailableSemaphores.Length(); i++)
    {
        if (imageAvailableSemaphores[i])
        {
            vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
            imageAvailableSemaphores[i] = VK_NULL_HANDLE;
        }
        if (renderFinishedSemaphores[i])
        {
            vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
            renderFinishedSemaphores[i] = VK_NULL_HANDLE;
        }
    }
    imageAvailableSemaphores.RemoveAll();
    renderFinishedSemaphores.RemoveAll();
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
      inFlightFence(),
      currentImageIndex(0),
      frameSemaphoreIndex(0),
      physicalDevice(),
      device(),
      graphicsQueue(),
      presentQueue(),
      graphicsQueueFamilyIndex(UINT32_MAX),
      presentQueueFamilyIndex(UINT32_MAX),
      pActiveCamera(nullptr),
      renderPass(),
      swapchainFramebuffers(),
      pipelineLayout(),
      graphicsPipeline(),
      vertShaderModule(),
      fragShaderModule(),
      descriptorSetLayout(),
      descriptorPool(),
      descriptorSet(),
      uniformBuffer(),
      uniformBufferMemory(),
      textureImage(),
      textureImageMemory(),
      textureImageView(),
      textureSampler(),
      textureReady(false),
      depthImage(VK_NULL_HANDLE),
      depthImageMemory(VK_NULL_HANDLE),
      depthImageView(VK_NULL_HANDLE),
      renderableGpuMap()
{
}

VulkanRenderer::~VulkanRenderer()
{
    CleanupRenderableGpuMap();
    CleanupPipelineResources();
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
    {
        PRINTLN_ERR("VulkanRenderer: required instance extensions not supported.");
        return false;
    }

    // ── 2. Validation layers ───────────────────────────────────────────
    DynamicArray<const char *> validationLayers;
    uint32_t nValidationLayers = 0;
#ifndef NDEBUG
    validationLayers.Append("VK_LAYER_KHRONOS_validation");
    nValidationLayers = validationLayers.Length();
    if (!CheckSupportedValidationLayers(&validationLayers))
    {
        // validation layer 是可選的:沒有就警告並繼續(不讓 Debug build 卡死)
        PRINTLN_ERR("VulkanRenderer: VK_LAYER_KHRONOS_validation not available — continuing without validation.");
        validationLayers.RemoveAll();
        nValidationLayers = 0;
    }
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
    {
        PRINTLN_ERR("VulkanRenderer: failed to create instance.");
        return false;
    }

    if (!SetupDebugMessenger())
    {
        PRINTLN_ERR("VulkanRenderer: failed to set up debug messenger.");
        return false;
    }

    // ── 4. Create surface ──────────────────────────────────────────────
    if (!CreateSurface(pWindow))
    {
        PRINTLN_ERR("VulkanRenderer: failed to create window surface.");
        return false;
    }

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

    // ── 8b. Depth buffer(需要 swapchain extent)──────────────────────────
    if (!CreateDepthResources())
    {
        PRINTLN_ERR("VulkanRenderer: failed to create depth resources.");
        return false;
    }

    // ── 9. Create command pool + buffers ───────────────────────────────
    if (!CreateCommandPool())
    {
        PRINTLN_ERR("VulkanRenderer: failed to create command pool.");
        return false;
    }
    if (!CreateCommandBuffers())
    {
        PRINTLN_ERR("VulkanRenderer: failed to create command buffers.");
        return false;
    }

    // ── 10. Create sync objects (semaphores + fence) ───────────────────
    if (!CreateSyncObjects())
    {
        PRINTLN_ERR("VulkanRenderer: failed to create synchronization objects.");
        return false;
    }

    // ── 11. Render pass / framebuffers / pipeline ───────────────────────
    if (!CreateRenderPass() || !CreateFramebuffers() || !CreateDescriptorSetLayout() ||
        !CreateDescriptorPool() || !CreateGraphicsPipeline() || !CreateUniformBuffer() ||
        !CreateWhiteTexture())
    {
        PRINTLN_ERR("VulkanRenderer: failed to create pipeline resources.");
        return false;
    }

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
    // swapchain / framebuffer 依賴 surface 尺寸,resize 後重建
    // framebuffer 參照 swapchain image view,必須先於 swapchain 銷毀;
    // depth buffer 依附 extent,一併重建
    vkDeviceWaitIdle(device);
    CleanupFramebuffers();
    CleanupDepthResources();
    CleanupSwapchain();
    CreateSwapchain();
    CreateSwapchainImageViews();
    CreateDepthResources();
    CreateFramebuffers();
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
    if (!device)
        return true; // 未初始化(如子視窗從未 Show):跳過本幀,避免空指標崩潰

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
                if (command.pRenderable)
                {
                    LoadRenderable(*command.pRenderable);
                    RecordDrawCommands(commandBuffers[currentImageIndex], *command.pRenderable);
                }
                break;
            case Frame::Command::DrawGUILayout:
                if (command.pLayout)
                {
                    // P6(a):GUI 走 Frame — 佈局 = layers + 各自 components,全都是
                    // Rectangle(IRenderable),走與 DrawRenderable 相同的 renderable 管線。
                    // 正交投影的實際像素填充留待 P6(b)(需桌面人工驗收)。
                    const DynamicArray<SharedPtr<GUILayer>> &layers = command.pLayout->GetLayers();
                    for (size_t li = 0; li < layers.GetNElements(); li++)
                    {
                        LoadRenderable(*layers[li]);
                        RecordDrawCommands(commandBuffers[currentImageIndex], *layers[li]);
                        const DynamicArray<SharedPtr<IGUI>> &components = layers[li]->GetComponents();
                        for (size_t ci = 0; ci < components.GetNElements(); ci++)
                        {
                            LoadRenderable(*components[ci]);
                            RecordDrawCommands(commandBuffers[currentImageIndex], *components[ci]);
                        }
                    }
                }
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
    // 用上一幀 image 的 semaphore 組 acquire:該 image 的 present 已被
    // vkWaitForFences 保證完成,組可安全重用
    frameSemaphoreIndex = currentImageIndex;
    const VkResult acquireResult = vkAcquireNextImageKHR(
        device, swapchain, UINT64_MAX, imageAvailableSemaphores[frameSemaphoreIndex], VK_NULL_HANDLE, &imageIndex);
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

    // 以 render pass 開場(loadOp = CLEAR,深藍黑清色)
    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapchainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapchainExtent;
    VkClearValue clearValues[2] = {};
    clearValues[0].color = {{0.02f, 0.04f, 0.08f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};
    renderPassInfo.clearValueCount = 2;
    renderPassInfo.pClearValues = clearValues;
    vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    return true;
}

bool VulkanRenderer::EndFrame()
{
    VkCommandBuffer cmdBuffer = commandBuffers[currentImageIndex];
    vkCmdEndRenderPass(cmdBuffer);
    if (vkEndCommandBuffer(cmdBuffer) != VK_SUCCESS)
        return false;

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[frameSemaphoreIndex]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[frameSemaphoreIndex]};

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

// ══════════════════════════════════════════════════════════════════════════════
//  Pipeline 建置
// ══════════════════════════════════════════════════════════════════════════════

bool VulkanRenderer::CreateRenderPass()
{
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = swapchainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // depth attachment(深度測試 / 寫入)
    VkAttachmentDescription depthAttachment = {};
    depthAttachment.format = VK_FORMAT_D32_SFLOAT;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef = {};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    // extern → 子通道開頭:wait color attachment(等待 clear prepare)
    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkAttachmentDescription attachments[2] = {colorAttachment, depthAttachment};

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 2;
    renderPassInfo.pAttachments = attachments;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
    {
        PRINTLN_ERR(L"VulkanRenderer: failed to create render pass.");
        return false;
    }
    return true;
}

bool VulkanRenderer::CreateFramebuffers()
{
    swapchainFramebuffers.Resize(swapchainImageViews.Length());
    VkExtent2D extent = swapchainExtent;
    for (size_t i = 0; i < swapchainImageViews.Length(); i++)
    {
        VkImageView attachments[] = {swapchainImageViews[i], depthImageView};
        VkFramebufferCreateInfo fbInfo = {};
        fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbInfo.renderPass = renderPass;
        fbInfo.attachmentCount = 2;
        fbInfo.pAttachments = attachments;
        fbInfo.width = extent.width;
        fbInfo.height = extent.height;
        fbInfo.layers = 1;
        if (vkCreateFramebuffer(device, &fbInfo, nullptr, &swapchainFramebuffers[i]) != VK_SUCCESS)
            return false;
    }
    return true;
}

void VulkanRenderer::CleanupFramebuffers()
{
    if (!device)
        return;
    for (size_t i = 0; i < swapchainFramebuffers.Length(); i++)
        vkDestroyFramebuffer(device, swapchainFramebuffers[i], nullptr);
    swapchainFramebuffers.RemoveAll();
}

void VulkanRenderer::CleanupDepthResources()
{
    if (!device)
        return;
    if (depthImageView)
        vkDestroyImageView(device, depthImageView, nullptr);
    if (depthImage)
        vkDestroyImage(device, depthImage, nullptr);
    if (depthImageMemory)
        vkFreeMemory(device, depthImageMemory, nullptr);
    depthImageView = VK_NULL_HANDLE;
    depthImage = VK_NULL_HANDLE;
    depthImageMemory = VK_NULL_HANDLE;
}

bool VulkanRenderer::CreateDepthResources()
{
    // D32_SFLOAT:Pascal(GT 1030)必支援;深度不需要 stencil
    const VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = {swapchainExtent.width, swapchainExtent.height, 1};
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = depthFormat;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateImage(device, &imageInfo, nullptr, &depthImage) != VK_SUCCESS)
        return false;

    VkMemoryRequirements memReq;
    vkGetImageMemoryRequirements(device, depthImage, &memReq);
    const uint32_t memType = FindMemoryType(physicalDevice, memReq.memoryTypeBits,
                                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (memType == UINT32_MAX)
        return false;
    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = memType;
    if (vkAllocateMemory(device, &allocInfo, nullptr, &depthImageMemory) != VK_SUCCESS)
        return false;
    if (vkBindImageMemory(device, depthImage, depthImageMemory, 0) != VK_SUCCESS)
        return false;

    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = depthImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = depthFormat;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.layerCount = 1;
    return vkCreateImageView(device, &viewInfo, nullptr, &depthImageView) == VK_SUCCESS;
}

bool VulkanRenderer::CreateDescriptorSetLayout()
{
    // binding 0: UBO(vertex + fragment — fragment shader 也讀 useTexture,
    // 只設 VERTEX 會讓 pipeline 違反 spec,useTexture 變成未定義值)
    VkDescriptorSetLayoutBinding uboBinding = {};
    uboBinding.binding = 0;
    uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboBinding.descriptorCount = 1;
    uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    // binding 1: 組合的取樣器(fragment)
    VkDescriptorSetLayoutBinding samplerBinding = {};
    samplerBinding.binding = 1;
    samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerBinding.descriptorCount = 1;
    samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding bindings[] = {uboBinding, samplerBinding};
    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 2;
    layoutInfo.pBindings = bindings;
    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
        return false;
    return true;
}

bool VulkanRenderer::CreateDescriptorPool()
{
    VkDescriptorPoolSize poolSizes[] = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}
    };
    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 2;
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.maxSets = 1;
    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
        return false;
    return true;
}

bool VulkanRenderer::CreateShaderModule(const unsigned char *pCode, size_t codeSize, VkShaderModule &outModule)
{
    // xxd 產生的是 byte array;複製到對齊良好的 uint32 buffer
    DynamicArray<uint32_t> aligned;
    aligned.Resize(codeSize / sizeof(uint32_t) + 1);
    memset(&aligned[0], 0, aligned.Length());
    memcpy(&aligned[0], pCode, codeSize);

    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = codeSize;
    createInfo.pCode = &aligned[0];
    if (vkCreateShaderModule(device, &createInfo, nullptr, &outModule) != VK_SUCCESS)
        return false;
    return true;
}

bool VulkanRenderer::CreateGraphicsPipeline()
{
    if (!CreateShaderModule(default_vert_spv, default_vert_spv_len, vertShaderModule))
        return false;
    if (!CreateShaderModule(default_frag_spv, default_frag_spv_len, fragShaderModule))
        return false;

    VkPipelineShaderStageCreateInfo shaderStages[2] = {};
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = vertShaderModule;
    shaderStages[0].pName = "main";
    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = fragShaderModule;
    shaderStages[1].pName = "main";

    // Vertex input:interleaved VulkanVertex
    VkVertexInputBindingDescription bindingDesc = {};
    bindingDesc.binding = 0;
    bindingDesc.stride = sizeof(VulkanVertex);
    bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attributeDescs[5] = {};
    attributeDescs[0].location = 0; attributeDescs[0].binding = 0;
    attributeDescs[0].format = VK_FORMAT_R32G32B32A32_SFLOAT; attributeDescs[0].offset = 0;                    // position
    attributeDescs[1].location = 1; attributeDescs[1].binding = 0;
    attributeDescs[1].format = VK_FORMAT_R32G32B32A32_SFLOAT; attributeDescs[1].offset = 16;                   // color
    attributeDescs[2].location = 2; attributeDescs[2].binding = 0;
    attributeDescs[2].format = VK_FORMAT_R32G32_SFLOAT;       attributeDescs[2].offset = 32;                   // texCoord
    attributeDescs[3].location = 3; attributeDescs[3].binding = 0;
    attributeDescs[3].format = VK_FORMAT_R32_UINT;            attributeDescs[3].offset = 40;                   // cmode
    attributeDescs[4].location = 4; attributeDescs[4].binding = 0;
    attributeDescs[4].format = VK_FORMAT_R32_UINT;            attributeDescs[4].offset = 44;                   // gui

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
    vertexInputInfo.vertexAttributeDescriptionCount = 5;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescs;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE; // 不剔除,避免頂點繞序問題(v1)
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlend = {};
    colorBlend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlend.blendEnable = VK_TRUE;
    colorBlend.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlend.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlend.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlend;

    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
        return false;

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
        return false;
    return true;
}

bool VulkanRenderer::CreateUniformBuffer()
{
    return CreateBuffer(device, physicalDevice, sizeof(MatrixBuffer),
                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        uniformBuffer, uniformBufferMemory);
}

VkImageView VulkanRenderer::CreateImageView(VkImage image, VkFormat format)
{
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.layerCount = 1;
    VkImageView view = VK_NULL_HANDLE;
    vkCreateImageView(device, &viewInfo, nullptr, &view);
    return view;
}

bool VulkanRenderer::CreateWhiteTexture()
{
    // sampler
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerInfo.maxLod = 1.0f;
    if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS)
        return false;

    // 1x1 白色 fallback 貼圖(沒有真實貼圖時也讓 binding 1 有效)
    const uint32_t whitePixel = 0xFFFFFFFF;
    if (!CreateImageWithData(device, physicalDevice, graphicsQueue, commandPool,
                             1, 1, &whitePixel, textureImage, textureImageMemory))
        return false;
    textureImageView = CreateImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB);
    textureReady = true;

    // descriptor set:分配 + 寫入(UBO + sampler 都指向目前資源)
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptorSetLayout;
    vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet);

    UpdateTextureDescriptor();
    return true;
}

void VulkanRenderer::UpdateTextureDescriptor()
{
    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = uniformBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(MatrixBuffer);

    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = textureImageView;
    imageInfo.sampler = textureSampler;

    VkWriteDescriptorSet writes[2] = {};
    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].dstSet = descriptorSet;
    writes[0].dstBinding = 0;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[0].pBufferInfo = &bufferInfo;
    writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].dstSet = descriptorSet;
    writes[1].dstBinding = 1;
    writes[1].descriptorCount = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[1].pImageInfo = &imageInfo;
    vkUpdateDescriptorSets(device, 2, writes, 0, nullptr);
}

void VulkanRenderer::CleanupPipelineResources()
{
    if (!device)
        return;
    if (graphicsPipeline)
        vkDestroyPipeline(device, graphicsPipeline, nullptr);
    if (pipelineLayout)
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    if (vertShaderModule)
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
    if (fragShaderModule)
        vkDestroyShaderModule(device, fragShaderModule, nullptr);
    if (renderPass)
        vkDestroyRenderPass(device, renderPass, nullptr);
    CleanupFramebuffers();
    CleanupDepthResources();
    if (descriptorSetLayout)
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
    if (descriptorPool)
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    if (uniformBuffer)
        vkDestroyBuffer(device, uniformBuffer, nullptr);
    if (uniformBufferMemory)
        vkFreeMemory(device, uniformBufferMemory, nullptr);
    if (textureSampler)
        vkDestroySampler(device, textureSampler, nullptr);
    if (textureImageView)
        vkDestroyImageView(device, textureImageView, nullptr);
    if (textureImage)
        vkDestroyImage(device, textureImage, nullptr);
    if (textureImageMemory)
        vkFreeMemory(device, textureImageMemory, nullptr);
}

void VulkanRenderer::CleanupRenderableGpuMap()
{
    if (!device)
        return;
    for (auto itr = renderableGpuMap.First(); itr != renderableGpuMap.Last(); itr++)
    {
        vkDestroyBuffer(device, itr->Value().vertexBuffer, nullptr);
        vkFreeMemory(device, itr->Value().vertexMemory, nullptr);
    }
    renderableGpuMap.Clear();
}

// ══════════════════════════════════════════════════════════════════════════════
//  Renderable → GPU buffer / 繪製
// ══════════════════════════════════════════════════════════════════════════════

bool VulkanRenderer::LoadRenderable(const IRenderable &renderable)
{
    const size_t id = renderable.GetIdentifier();
    if (renderableGpuMap.Contains(id))
    {
        // 載入這顆 renderable 的貼圖(單一 slot,v1)
        // Texture::loaded 從不為 true(Texture 只是路徑容器),renderer 自己負責
        // 載入;LoadTextureImage 內部用 loadedTexturePath 去重
        const RenderInfo info = renderable.GetRenderInfo();
        if (info.pTexture && !textureReady)
            LoadTextureImage(*info.pTexture);
        return true;
    }

    const RenderInfo info = renderable.GetRenderInfo();
    const unsigned int nVertices = info.numOfVertices;
    if (!nVertices)
        return true;

    // 建 interleaved vertex array(與 OpenGL 版 CreateInputBuffer 相同慣例)
    const bool useIndex = info.vertexIndexBuffer && info.numOfVertexIndices > 0;
    // 有 index buffer 時展開成 numOfVertexIndices 個頂點:vkCmdDraw 是非 indexed,
    // 展開後才畫得到完整網格(cube 24 頂點 / 36 index → 36 展開頂點 = 12 三角形)。
    const unsigned int drawCount = useIndex ? info.numOfVertexIndices : nVertices;
    DynamicArray<VulkanVertex> vertices;
    vertices.Resize(drawCount);
    for (unsigned int i = 0; i < drawCount; i++)
    {
        size_t srcIdx = useIndex ? info.vertexIndexBuffer[i] : i;
        if (srcIdx >= info.numOfVertices)
            srcIdx = i;

        const Point3D &v = info.vertexBuffer[srcIdx];
        VulkanVertex vert;
        vert.position = glm::vec4(v.x, v.y, v.z, 1.0f);

        if (info.colorBuffer && info.numOfColors > 0)
        {
            size_t cIdx = info.colorIndexBuffer && i < info.numOfColorIndices ? info.colorIndexBuffer[i] : srcIdx;
            if (cIdx >= info.numOfColors)
                cIdx = 0;
            const Color &c = info.colorBuffer[cIdx];
            vert.color = glm::vec4(c.R, c.G, c.B, c.A);
        }
        else
        {
            vert.color = glm::vec4(1.0f);
        }

        if (info.textureCoordinates && i < info.numOfTextureCoordinates)
            vert.texCoord = glm::vec2(info.textureCoordinates[i].x, info.textureCoordinates[i].y);
        else
            vert.texCoord = glm::vec2(0.0f);

        vert.cmode = 1;
        vert.gui = 0;
        vertices[i] = vert;
    }

    RenderableGpuData data;
    data.vertexCount = drawCount;
    if (!CreateBuffer(device, physicalDevice, static_cast<VkDeviceSize>(drawCount) * sizeof(VulkanVertex),
                      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                      data.vertexBuffer, data.vertexMemory))
        return false;
    void *pMapped = nullptr;
    vkMapMemory(device, data.vertexMemory, 0, static_cast<VkDeviceSize>(drawCount) * sizeof(VulkanVertex), 0, &pMapped);
    memcpy(pMapped, &vertices[0], drawCount * sizeof(VulkanVertex));
    vkUnmapMemory(device, data.vertexMemory);

    renderableGpuMap.Insert(id, data);

    // 若這顆有貼圖,載入(場景裡通常只有一顆)
    if (info.pTexture)
        LoadTextureImage(*info.pTexture);

    return true;
}

bool VulkanRenderer::LoadTextureImage(const Texture &texture)
{
    int width = 0, height = 0, channels = 0;
    char pathBuf[1024];
    texture.imagePath.ToUTF8(pathBuf, 1024);
    if (!loadedTexturePath.empty() && loadedTexturePath == pathBuf)
        return true; // 這張已經載入

    const unsigned char *pPixels = stbi_load(pathBuf, &width, &height, &channels, 4);
    if (!pPixels)
    {
        PRINTLN_ERR(String(L"VulkanRenderer: failed to load texture '") + texture.imagePath + String(L"'."));
        return false;
    }

    if (textureImage)
        vkDestroyImage(device, textureImage, nullptr);
    if (textureImageMemory)
        vkFreeMemory(device, textureImageMemory, nullptr);
    textureImage = VK_NULL_HANDLE;
    textureImageMemory = VK_NULL_HANDLE;

    const bool ok = CreateImageWithData(device, physicalDevice, graphicsQueue, commandPool,
                                        static_cast<uint32_t>(width), static_cast<uint32_t>(height),
                                        pPixels, textureImage, textureImageMemory);
    stbi_image_free(const_cast<unsigned char *>(pPixels));
    if (!ok)
    {
        // 舊 image 已銷毀:重置 flags,避免 descriptor 指向已銷毀資源
        textureReady = false;
        loadedTexturePath.clear();
        return false;
    }

    if (textureImageView)
        vkDestroyImageView(device, textureImageView, nullptr);
    textureImageView = CreateImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB);
    textureReady = true;
    loadedTexturePath = pathBuf;
    UpdateTextureDescriptor();
    return true;
}

void VulkanRenderer::RecordDrawCommands(VkCommandBuffer cmdBuffer, const IRenderable &renderable)
{
    const size_t id = renderable.GetIdentifier();
    HashTable<size_t, RenderableGpuData>::Iterator itr = renderableGpuMap.Find(id);
    if (itr == renderableGpuMap.Last())
        return;
    const RenderableGpuData &data = itr->Value();

    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
                             0, 1, &descriptorSet, 0, nullptr);

    // UBO:逐 drawable 更新(world / view / proj)
    const Point3D &pos = renderable.GetPosition();
    const Point3D &rot = renderable.GetRotation();
    const Point3D &scale = renderable.GetScale();
    const glm::mat4 world = BuildWorldMatrix(pos, rot, scale);

    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::perspective(glm::radians(70.0f),
                                             static_cast<float>(swapchainExtent.width) /
                                                 static_cast<float>(swapchainExtent.height),
                                             0.001f, 100.0f);
    if (pActiveCamera)
    {
        view = BuildViewMatrix(pActiveCamera->GetPosition(), pActiveCamera->GetRotation());
        const float aspect = static_cast<float>(swapchainExtent.width) /
                             static_cast<float>(swapchainExtent.height);
        projection = BuildProjMatrix(pActiveCamera->GetAngleOfView(), aspect,
                                     pActiveCamera->GetDistanceToNearPlane(),
                                     pActiveCamera->GetDistanceToFarPlane());
    }
    const bool useTexture = textureReady && renderable.GetRenderInfo().pTexture != nullptr;
    UpdateUniformBuffer(world, view, projection, useTexture);

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapchainExtent.width);
    viewport.height = static_cast<float>(swapchainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = swapchainExtent;
    vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

    VkBuffer vertexBuffers[] = {data.vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmdBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdDraw(cmdBuffer, data.vertexCount, 1, 0, 0);
}

void VulkanRenderer::UpdateUniformBuffer(const glm::mat4 &world, const glm::mat4 &view,
                                         const glm::mat4 &projection, bool useTexture)
{
    MatrixBuffer matrices;
    matrices.world = world;
    matrices.view = view;
    matrices.projection = projection;
    matrices.useTexture = useTexture ? 1.0f : 0.0f;

    void *pData = nullptr;
    vkMapMemory(device, uniformBufferMemory, 0, sizeof(MatrixBuffer), 0, &pData);
    memcpy(pData, &matrices, sizeof(MatrixBuffer));
    vkUnmapMemory(device, uniformBufferMemory);
}
