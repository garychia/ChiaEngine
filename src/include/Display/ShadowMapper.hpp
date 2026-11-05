#ifndef SHADOW_MAPPER_HPP
#define SHADOW_MAPPER_HPP

#include <cstdint>

#if !defined(VULKAN_ENABLED) && !defined(DIRECTX_ENABLED)
#include <GL/gl.h>
#endif

// ShadowMapper:為方向光建立深度貼圖(FBO + depth texture)。
// 僅在 OpenGL 後端編譯；VULKAN/DIRECTX 後端不編譯此類別。
//
// 流程:
//  1. Initialize():建立 FBO、深度貼圖(預設 1024x1024)
//  2. BeginShadowPass(lightDir):綁定 FBO、設定正交投影、清除深度
//  3. EndShadowPass():解綁 FBO、回復預設幀緩衝
//  4. BindForReading(textureUnit):將深度貼圖綁定到指定紋理單元供著色器取樣
class ShadowMapper
{
  public:
    ShadowMapper();
    ~ShadowMapper();

    // 初始化 OpenGL 資源(FBO + 深度貼圖)。需要當前線程有當前 OpenGL 上下文。
    bool Initialize(int width = 1024, int height = 1024);

    // 開始陰影通道:綁定 FBO、設定視口、清除深度緩衝。
    // 光源方向用於後續建立光空間矩陣(此處僅記錄，矩陣由 ForwardRenderer 組裝)。
    void BeginShadowPass();

    // 結束陰影通道:解綁 FBO。
    void EndShadowPass();

    // 將深度貼圖綁定到指定紋理單元(供著色器採樣)。
    void BindForReading(GLuint textureUnit = 1);

    // 釋放 OpenGL 資源。
    void Shutdown();

    GLuint GetDepthTexture() const { return mDepthTexture; }
    GLuint GetFBO() const { return mFBO; }
    int GetWidth() const { return mWidth; }
    int GetHeight() const { return mHeight; }
    bool IsInitialized() const { return mInitialized; }

  private:
    GLuint mFBO = 0;
    GLuint mDepthTexture = 0;
    int mWidth = 1024;
    int mHeight = 1024;
    bool mInitialized = false;
};

#endif // SHADOW_MAPPER_HPP
