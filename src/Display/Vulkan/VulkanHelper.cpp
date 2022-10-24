#include "Display/Vulkan/VulkanHelper.hpp"

bool VulkanHelper::CreateInstance(const char *pAppName, const Version &appVersion, const char *pEngineName,
                                  const Version &engineVersion, uint32_t nExtensions, const char **ppExtensionNames,
                                  uint32_t nValidationLayers, const char **ppValidationLayerNames,
                                  const VkAllocationCallbacks *pAllocationCallbacks,
                                  VkInstance *pInstance)
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
