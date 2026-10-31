#include "Display/Asset/TextureAssetView.hpp"

// stb_image:僅此一個 TU 負責實作 implementation,避免多重定義。
#define STB_IMAGE_IMPLEMENTATION
#include "3rdparty/stb_image.h"

#include <cstring>

bool DecodeImageFromMemory(const unsigned char *buffer, size_t len, int *outW, int *outH,
                           DynamicArray<unsigned char> *outRGBA)
{
    if (!buffer || len == 0 || !outW || !outH || !outRGBA)
        return false;

    int w = 0, h = 0, ch = 0;
    // req_comp = 4 → 強制輸出 RGBA(4 通道,未乘积 alpha),統一 renderer 端的上傳格式。
    unsigned char *pixels = stbi_load_from_memory(buffer, static_cast<int>(len), &w, &h, &ch, 4);
    if (!pixels)
        return false;

    const size_t n = static_cast<size_t>(w) * static_cast<size_t>(h) * 4u;
    outRGBA->Resize(n);
    std::memcpy(&(*outRGBA)[0], pixels, n);
    stbi_image_free(pixels);

    *outW = w;
    *outH = h;
    return true;
}

void TextureAssetView::OnAssetLoaded(SharedPtr<Asset> asset)
{
    if (!hook || !asset || !asset->loaded || !asset->pBlock)
        return;

    const DynamicArray<unsigned char> &rawBytes = asset->pBlock->bytes;
    const size_t contentHash = asset->ContentHash();

    int w = 0, h = 0;
    DynamicArray<unsigned char> rgba;
    bool ok = false;
    if (rawBytes.Length() > 0)
        ok = DecodeImageFromMemory(&rawBytes[0], rawBytes.Length(), &w, &h, &rgba);

    if (ok)
        hook(&rgba[0], w, h, contentHash, userData);
    else
        hook(nullptr, 0, 0, contentHash, userData); // 解碼失敗:renderer 得知該 asset 不可用
}
