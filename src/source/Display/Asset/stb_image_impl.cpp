// 唯一的 stb_image 實作翻譯單元,避免在 TextureAssetView.cpp 與
// VulkanRenderer.cpp 中重複定義 STB_IMAGE_IMPLEMENTATION 導致多重定義。
#define STB_IMAGE_IMPLEMENTATION
#include "3rdparty/stb_image.h"
