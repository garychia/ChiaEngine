#include "Display/Vulkan/VulkanHelper.hpp"

bool VulkanHelper::CreateInstance(const char *pAppName, const Version &appVersion, const char *pEngineName,
                                  const Version &engineVersion, uint32_t nExtensions, const char **ppExtensionNames,
                                  uint32_t nValidationLayers, const char **ppValidationLayerNames,
                                  const VkAllocationCallbacks *pAllocationCallbacks, VkInstance *pInstance)
{
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = pAppName;
    appInfo.applicationVersion = VK_MAKE_VERSION(appVersion.major, appVersion.minor, appVersion.patch);
    appInfo.pEngineName = pEngineName;
    appInfo.engineVersion = VK_MAKE_VERSION(engineVersion.major, engineVersion.minor, engineVersion.patch);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo instanceInfo{};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pApplicationInfo = &appInfo;
    instanceInfo.enabledExtensionCount = nExtensions;
    instanceInfo.ppEnabledExtensionNames = ppExtensionNames;
    instanceInfo.enabledLayerCount = nValidationLayers;
    instanceInfo.ppEnabledLayerNames = ppValidationLayerNames;
    VkResult result = vkCreateInstance(&instanceInfo, pAllocationCallbacks, pInstance);
    return result == VK_SUCCESS;
}

bool VulkanHelper::ListSupportedExtensions(const char *pLayerNmae, uint32_t *nExtensions,
                                           DynamicArray<VkExtensionProperties> *pExtensionProperties)
{
    const auto result = vkEnumerateInstanceExtensionProperties(pLayerNmae, nExtensions, nullptr);
    if (VK_SUCCESS != result)
        return false;
    pExtensionProperties->Resize(*nExtensions);
    return VK_SUCCESS == vkEnumerateInstanceExtensionProperties(pLayerNmae, nExtensions, &(*pExtensionProperties)[0]);
}

bool VulkanHelper::ListSupportedValidationLayers(uint32_t *pNLayers, DynamicArray<VkLayerProperties> *pLayers)
{
    const auto result = vkEnumerateInstanceLayerProperties(pNLayers, nullptr);
    if (VK_SUCCESS != result)
        return false;
    pLayers->Resize(*pNLayers);
    return VK_SUCCESS == vkEnumerateInstanceLayerProperties(pNLayers, &(*pLayers)[0]);
}

VkResult VulkanHelper::CreateDebugUtilsMessengerEXT(VkInstance instance,
                                                    const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                                    const VkAllocationCallbacks *pAllocator,
                                                    VkDebugUtilsMessengerEXT *pDebugMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    return func ? func(instance, pCreateInfo, pAllocator, pDebugMessenger) : VK_ERROR_EXTENSION_NOT_PRESENT;
}

void VulkanHelper::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                                 const VkAllocationCallbacks *pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func)
        func(instance, debugMessenger, pAllocator);
}
