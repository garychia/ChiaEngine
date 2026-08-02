#ifndef IFRAME_EXECUTOR_HPP
#define IFRAME_EXECUTOR_HPP

#include "Frame.hpp"

class Window;

// 圖形後端的統一介面:每一幀 = 執行一份 Frame。
// 取代目前肥大、每後端各寫一套的 IRenderer — 後端差異被壓到最小:
// Vulkan / DirectX / OpenGL 各自只需要「把命令翻譯成 API 呼叫」。
class IFrameExecutor
{
  public:
    virtual ~IFrameExecutor() = default;

    virtual bool Initialize(const Window *pWindow) = 0;

    virtual bool Execute(const Frame &frame) = 0;

    virtual void OnWindowResized(long newWidth, long newHeight) = 0;
};

#endif // IFRAME_EXECUTOR_HPP
