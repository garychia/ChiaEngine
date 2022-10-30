#ifndef VULKAN_HELPER_HPP
#define VULKAN_HELPER_HPP

#include "Data/DynamicArray.hpp"
#include "Version.hpp"
#include "pch.hpp"

class VulkanHelper
{
  public:
    static bool CreateInstance(const char *pAppName, const Version &appVersion, const char *pEngineName,
                               const Version &engineVersion, uint32_t nExtensions, const char **ppExtensionNames,
                               uint32_t nValidationLayers, const char **ppValidationLayerNames,
                               const VkAllocationCallbacks *pAllocationCallbacks, VkInstance *pInstance);

    static bool ListSupportedExtensions(const char *pLayerNmae, uint32_t *pNExtensions,
                                        DynamicArray<VkExtensionProperties> *pExtensionProperties);

    static bool ListSupportedValidationLayers(uint32_t *pNLayers, DynamicArray<VkLayerProperties> *pLayers);

    static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
                                                 const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                                 const VkAllocationCallbacks *pAllocator,
                                                 VkDebugUtilsMessengerEXT *pDebugMessenger);

    static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                              const VkAllocationCallbacks *pAllocator);
};

#endif
