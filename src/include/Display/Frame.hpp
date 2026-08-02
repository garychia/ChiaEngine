#ifndef FRAME_HPP
#define FRAME_HPP

#include "Data/DynamicArray.hpp"
#include "Data/Pointers.hpp"

#include <cstddef>

class Camera;
class GUILayout;
class IRenderable;

// 渲染命令流:Sim/View 產出,IFrameExecutor 執行。
// Frame 是「模擬」與「呈現」之間唯一的貨幣 — 兩層不互相呼叫,只交換 Frame。
//
// 典型錄製順序:
//   frame.BeginFrame();
//   frame.SetCamera(scene.GetCamera());
//   for (auto &renderable : scene) frame.DrawRenderable(*renderable);
//   frame.EndFrame();
//   executor.Execute(frame);
class Frame
{
  public:
    enum class Command
    {
        BeginFrame,     // 幀起點:清畫面、acquire swapchain image
        SetCamera,      // 設定本幀相機
        DrawRenderable, // 畫一個 IRenderable
        DrawGUILayout,  // 畫 GUI 佈局
        EndFrame        // 幀終點:提交 + 呈現
    };

    // 命令酬載:未使用的欄位保證為 nullptr。
    struct CommandData
    {
        Command command;
        Camera *pCamera;
        const IRenderable *pRenderable;
        GUILayout *pLayout;
    };

  private:
    DynamicArray<CommandData> commands;

    void Append(Command command)
    {
        CommandData data;
        data.command = command;
        data.pCamera = nullptr;
        data.pRenderable = nullptr;
        data.pLayout = nullptr;
        commands.Append(data);
    }

  public:
    Frame() : commands()
    {
    }

    void BeginFrame()
    {
        Append(Command::BeginFrame);
    }

    void SetCamera(WeakPtr<Camera> pCamera)
    {
        Append(Command::SetCamera);
        commands.GetLast().pCamera = pCamera.operator->(); // 無效相機 → nullptr(安全)
    }

    void DrawRenderable(const IRenderable &renderable)
    {
        Append(Command::DrawRenderable);
        commands.GetLast().pRenderable = &renderable;
    }

    void DrawGUILayout(GUILayout &layout)
    {
        Append(Command::DrawGUILayout);
        commands.GetLast().pLayout = &layout;
    }

    void EndFrame()
    {
        Append(Command::EndFrame);
    }

    void Clear()
    {
        commands.RemoveAll();
    }

    size_t GetNumCommands() const
    {
        return commands.GetNElements();
    }

    const CommandData &GetCommand(size_t index) const
    {
        return commands[index];
    }
};

#endif // FRAME_HPP
