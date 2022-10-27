#ifndef VULKAN_HELPER_HPP
#define VULKAN_HELPER_HPP

#include "Version.hpp"
#include "pch.hpp"

class VulkanHelper
{
  public:
    static bool CreateInstance(const char *pAppName, const Version &appVersion, const char *pEngineName,
                               const Version &engineVersion, uint32_t nExtensions, const char **ppExtensionNames,
                               uint32_t nValidationLayers, const char **ppValidationLayerNames,
                               const VkAllocationCallbacks *pAllocationCallbacks, VkInstance *pInstance);
};

#endif
