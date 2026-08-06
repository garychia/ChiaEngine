#ifndef IFRAME_EXECUTOR_HPP
#define IFRAME_EXECUTOR_HPP

#include "Frame.hpp"

class Window;

// 圖形後端的統一介面:每一幀 = 執行一份 Frame。
// 取代目前肥大、每後端各寫一套的 IRenderer — 後端差異被壓到最小:
// Vulkan / DirectX / OpenGL 各自只需要「把命令翻譯成 API 呼叫」。
//
// P7d:介面不變(Execute 已是唯一核心方法);新增命令是命令語彙的一部分:
//   PushTransform / PopTransform 由 executor 維護 transform stack(深度上限 64);
//   BindMaterial / DrawMesh 以 content-hash id 為 key 的快取;
//   SetViewport 設視口;DrawText 為 MVP stub。實作契約見 Frame::Command。
class IFrameExecutor
{
  public:
    virtual ~IFrameExecutor() = default;

    virtual bool Initialize(const Window *pWindow) = 0;

    virtual bool Execute(const Frame &frame) = 0;

    virtual void OnWindowResized(long newWidth, long newHeight) = 0;
};

#endif // IFRAME_EXECUTOR_HPP
