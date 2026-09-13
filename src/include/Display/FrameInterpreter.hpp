#ifndef FRAME_INTERPRETER_HPP
#define FRAME_INTERPRETER_HPP

// FrameInterpreter — 介於 Frame 與 GPU recording 之間的純解譯器(issue #84)。
//
// 目的:把「Frame 命令流 → 每幀繪製決策」這個純邏輯從 VulkanRenderer 抽出,
// 讓它不依賴任何 Vulkan/GLFW 型別,可在 host 上 standalone 測試。VulkanRenderer
// 原本的 Execute 是同一個 switch 混著純邏輯(transform stack 上限 64、empty-pop
// no-op、identity fallback、null 指標防護、material/camera 狀態)與 GPU 呼叫
// (vkCmd*/Record*),那批純邏輯是死碼外的可測核心 — 移到這裡。
//
// 職責:
//   - 逐命令走 Frame,維護 executor 端狀態:transform stack(深度上限 64)、
//     active camera、bound material。
//   - 把命令攤平成 RenderOp 串流,每個 op 帶「已解析」的 world matrix
//     (PushTransform 當下 BuildWorld;DrawMesh/DrawText 用 stack top,空 stack
//      回傳 identity)。
//   - legacy 指標命令(SetCamera/DrawRenderable/DrawGUILayout)以 pass-through
//     op 保留(geometry 載入/錄製屬 GPU 端職責,layout 展開也在 GPU 端)。
//
// 契約(VulkanRenderer 原本行為,逐條保留):
//   - BeginFrame:重置 stack + bound material(幀間不殘留)。
//   - PushTransform:超過 MaxStackDepth 即忽略(防 Sim bug 溢出)。
//   - PopTransform:空 stack 為 no-op。
//   - DrawMesh / DrawText:stack top 的 world;空 stack 用 identity。
//
// 此檔是純邏輯 + glm(RendererMath);不 include 任何 Display/Vulkan 或 GLFW
// 標頭 → host 可編譯(EMM:雙行標註,與 RendererContractTest 同型)。glm 為
// 純 header,standalone 測試只需 -I (glm 根),見 chiaengine-development skill
// 的 RendererContract standalone recipe。

#include "Display/Frame.hpp"
#include "Display/RendererMath.hpp"
#include "Display/Color.hpp"
#include "Data/DynamicArray.hpp"
#include "Display/GUI/GUILayout.hpp"
#include "Display/IRenderable.hpp"

#include <glm/glm.hpp>

#include <cstdint>

class Camera;
class GUILayout;

// 一筆「GPU 端可直接錄製」的標準化 op。值酬載已解析(world matrix 已算好);
// legacy 指標命令保留原始指標(GPU 端載入幾何用)。
struct RenderOp
{
    enum class Op
    {
        BeginFrame,      // 幀起點:清畫面、acquire
        EndFrame,        // 幀終點:submit + present
        SetCamera,       // 設定本幀相機(pCamera)
        DrawRenderable,  // 畫 IRenderable(pRenderable)
        DrawGUILayout,   // 畫 GUI 佈局(pLayout,GPU 端展開 layers/components)
        DrawMesh,        // 用 world + meshId + materialId 畫幾何
        SetViewport,     // 設視口(viewport)
        DrawText         // 用 world + fontId + text 畫文字
    };

    Op op = Op::BeginFrame;

    // ── legacy 指標(pass-through)──
    Camera *pCamera = nullptr;
    const IRenderable *pRenderable = nullptr;
    const GUILayout *pLayout = nullptr;

    // ── 值酬載(已解析)──
    glm::mat4 world = glm::mat4(1.0f); // DrawMesh/DrawText 的 resolved world
    uint64_t meshId = 0;
    uint64_t materialId = 0;           // DrawMesh 當下 BindMaterial 的狀態
    Frame::ViewportPayload viewport;   // SetViewport
    uint64_t fontId = 0;
    String text;                       // DrawText
    float textSize = 0;
    Color textColor;
};

class FrameInterpreter
{
  public:
    static constexpr size_t MaxStackDepth = 64; // 與原 VulkanRenderer 一致

    FrameInterpreter()
    {
        Reset();
    }

    // 清理並重置狀態(幀邊界、或 interpreter 重用時)。
    void Reset()
    {
        stack.RemoveAll();
        pActiveCamera = nullptr;
        boundMaterialId = 0;
    }

    // 攤平一幀 → RenderOp 串流。每次呼叫為一幀,輸入 Frame 在呼叫期間必須存活
    // (legacy 指標命令在 op 中仍指向原物件)。
    DynamicArray<RenderOp> Interpret(const Frame &frame)
    {
        DynamicArray<RenderOp> ops;
        const size_t count = frame.GetNumCommands();
        for (size_t i = 0; i < count; i++)
        {
            const Frame::CommandData &command = frame.GetCommand(i);
            switch (command.command)
            {
                case Frame::Command::BeginFrame:
                {
                    Reset();
                    RenderOp op;
                    op.op = RenderOp::Op::BeginFrame;
                    ops.Append(Types::Move(op));
                    break;
                }
                case Frame::Command::EndFrame:
                {
                    RenderOp op;
                    op.op = RenderOp::Op::EndFrame;
                    ops.Append(Types::Move(op));
                    break;
                }
                case Frame::Command::SetCamera:
                {
                    pActiveCamera = command.pCamera;
                    RenderOp op;
                    op.op = RenderOp::Op::SetCamera;
                    op.pCamera = pActiveCamera;
                    ops.Append(Types::Move(op));
                    break;
                }
                case Frame::Command::DrawRenderable:
                {
                    RenderOp op;
                    op.op = RenderOp::Op::DrawRenderable;
                    op.pRenderable = command.pRenderable;
                    ops.Append(Types::Move(op));
                    break;
                }
                case Frame::Command::DrawGUILayout:
                {
                    RenderOp op;
                    op.op = RenderOp::Op::DrawGUILayout;
                    op.pLayout = command.pLayout;
                    ops.Append(Types::Move(op));
                    break;
                }
                case Frame::Command::PushTransform:
                {
                    const Point3D &pos = command.transform.position;
                    const Point3D &rot = command.transform.rotation;
                    const Point3D &scale = command.transform.scale;
                    const glm::mat4 world = RendererMath::BuildWorldMatrix(pos, rot, scale);
                    if (stack.GetNElements() < FrameInterpreter::MaxStackDepth)
                        stack.Append(world);
                    break; // 超 64 忽略,不產 op
                }
                case Frame::Command::PopTransform:
                    if (!stack.IsEmpty())
                        stack.RemoveLast();
                    break; // 空 stack no-op,不產 op
                case Frame::Command::BindMaterial:
                    boundMaterialId = command.materialId;
                    break; // 狀態更新,無獨立 op(合併進 DrawMesh)
                case Frame::Command::DrawMesh:
                {
                    RenderOp op;
                    op.op = RenderOp::Op::DrawMesh;
                    op.world = stack.IsEmpty() ? glm::mat4(1.0f) : stack.GetLast();
                    op.meshId = command.meshId;
                    op.materialId = boundMaterialId;
                    ops.Append(Types::Move(op));
                    break;
                }
                case Frame::Command::SetViewport:
                {
                    RenderOp op;
                    op.op = RenderOp::Op::SetViewport;
                    op.viewport = command.viewport;
                    ops.Append(Types::Move(op));
                    break;
                }
                case Frame::Command::DrawText:
                {
                    RenderOp op;
                    op.op = RenderOp::Op::DrawText;
                    op.world = stack.IsEmpty() ? glm::mat4(1.0f) : stack.GetLast();
                    op.fontId = command.fontId;
                    op.text = command.text;
                    op.textSize = command.textSize;
                    op.textColor = command.textColor;
                    ops.Append(Types::Move(op));
                    break;
                }
            }
        }
        return ops;
    }

    // ── 契約可測的狀態端點 ──
    const Camera *GetActiveCamera() const { return pActiveCamera; }
    uint64_t GetBoundMaterialId() const { return boundMaterialId; }
    size_t GetStackDepth() const { return stack.GetNElements(); }

  private:
    DynamicArray<glm::mat4> stack; // PushTransform 推入的 world 矩陣(Executor 端狀態)
    Camera *pActiveCamera;
    uint64_t boundMaterialId;
};

#endif // FRAME_INTERPRETER_HPP