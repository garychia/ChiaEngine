set(VULKAN_SOURCE_FILES
    VulkanHelper.hpp
    VulkanHelper.cpp)

add_library(ChiaVulkan STATIC ${VULKAN_SOURCE_FILES})
target_include_directories(ChiaVulkan PUBLIC ${INCLUDE_DIR})
target_link_libraries(ChiaVulkan PUBLIC
    Vulkan::Vulkan
)
