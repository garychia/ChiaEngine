#include "Display/Vulkan/VulkanRenderer.hpp"

#include "App/App.hpp"
#include "Data/DynamicArray.hpp"
#include "Display/Vulkan/VulkanHelper.hpp"
#include "Display/Window.hpp"
#include "System/Debug/Debug.hpp"

#include <cstring>

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

VulkanRenderer::VulkanRenderer() : IRenderer(), vulkanInstance(), debugMessenger()
{
}

VulkanRenderer::~VulkanRenderer()
{
    VulkanHelper::DestroyDebugUtilsMessengerEXT(vulkanInstance, debugMessenger, NULL);
    vkDestroyInstance(vulkanInstance, nullptr);
}

bool VulkanRenderer::Initialize(const Window *pWindow)
{
    uint32_t nGLFWExtensions = 0;
    DynamicArray<const char *> pRequiredExtensionNames;
    auto pGLFWExtensions = glfwGetRequiredInstanceExtensions(&nGLFWExtensions);
    for (size_t i = 0; i < nGLFWExtensions; i++)
        pRequiredExtensionNames.Append(pGLFWExtensions[i]);

#ifdef __APPLE__
    glfwExtensionNames.Append(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif

#ifndef NDEBUG
    pRequiredExtensionNames.Append(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    if (!CheckSupportedExtensions(&pRequiredExtensionNames))
        return false;

    DynamicArray<const char *> validationLayers;
    uint32_t nValidationLayers = 0;
#ifndef NDEBUG
    validationLayers.Append("VK_LAYER_KHRONOS_validation");
    nValidationLayers = validationLayers.Length();
    if (!CheckSupportedValidationLayers(&validationLayers))
        return false;
#endif

    char appName[1024];
    const auto &winInfo = pWindow->GetWindowInfo();
    pWindow->GetWindowInfo().pAppInfo->appName.ToUTF8(appName, 1024);
    if (!VulkanHelper::CreateInstance(appName, winInfo.pAppInfo->appVersion, "Chia Engine",
                                      winInfo.pAppInfo->engineVersion, pRequiredExtensionNames.Length(),
                                      &pRequiredExtensionNames[0], nValidationLayers,
                                      nValidationLayers ? &validationLayers[0] : NULL, NULL, &vulkanInstance))
        return false;
    if (!SetupDebugMessenger())
        return false;
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
