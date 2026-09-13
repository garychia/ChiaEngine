#ifndef FRAME_INTERPRETER_TEST_HPP
#define FRAME_INTERPRETER_TEST_HPP

// FrameInterpreterTest(issue #84 — 「contract test targets the real seam」):
// 直接測試真正的 FrameInterpreter(純解譯器,Frame → 標準化 RenderOp),取代
// 過去只能靠 hand-rolled MockExecutor 複製/驗證的狀況。FrameInterpreter 是
// VulkanRenderer::Execute 抽出的純邏輯層(transform stack 64 上限、empty-pop
// no-op、identity fallback、material/camera 狀態、world 解析),完全 host 可測。
//
// 覆蓋範圍:
//   1. op dispatch:7 種 op 依錄製順序輸出(RenderOp::Op + payload fidelity)。
//   2. transform stack 契約:Depth 64 上限、第 65 個 push 忽略、empty-pop no-op、
//      BeginFrame 重置、跨幀不殘留。
//   3. identity fallback:DrawMesh/DrawText 空 stack → world = identity。
//   4. global 狀態檔案:SetCamera(active camera)、BindMaterial→DrawMesh 的
//      materialId 解析。
//   5. world 解析:PushTransform 當下用 RendererMath::BuildWorldMatrix 合成,
//      DrawMesh/DrawText 取 stack top(逐元素等值比較)。
//
// 全檔包在 namespace frameinterpretertest:所有 test header 編譯進同一個 TU。

#include "Test.hpp"
#include "Display/FrameInterpreter.hpp"
#include "Display/RendererMath.hpp"
#include "Display/Frame.hpp"
#include "Display/Camera.hpp"
#include "Display/IRenderable.hpp"
#include "Data/Pointers.hpp"

#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>

#include <cmath>

namespace frameinterpretertest
{

// 等值比較 helper:glm::mat4 欄對欄比對(測試用,不等於 operator==,因浮點誤差)。
static bool MatEqual(const glm::mat4 &a, const glm::mat4 &b, float eps = 1e-4f)
{
    for (int c = 0; c < 4; c++)
        for (int r = 0; r < 4; r++)
            if (std::fabs(a[c][r] - b[c][r]) > eps)
                return false;
    return true;
}

static bool IsIdentity(const glm::mat4 &m)
{
    return MatEqual(m, glm::mat4(1.0f));
}

class FrameInterpreterTest : public Test
{
  public:
    FrameInterpreterTest() : Test("FrameInterpreter")
    {
    }

    bool Run() noexcept override
    {
        // ═══ 1. op dispatch:順序 + payload fidelity ═════════════════════════
        TEST_MESSAGE("FrameInterpreter: op dispatch order + payload fidelity");

        {
            Frame frame;
            SharedPtr<Camera> pCamera = SharedPtr<Camera>::Construct(Point3D(10, 20, 30), Point3D(0, 90, 0));
            IRenderable renderable;
            frame.BeginFrame();
            frame.SetCamera(WeakPtr<Camera>(pCamera));
            frame.DrawRenderable(renderable);
            frame.PushTransform(Point3D(1, 2, 3), Point3D(0, 45, 0), Point3D(1, 2, 1));
            frame.BindMaterial(0xABCULL);
            frame.DrawMesh(0x1234ULL);
            frame.SetViewport(12.5f, 13.5f, 800.0f, 600.0f);
            frame.DrawText(9, String(u"Hi"), 14.5f, Color(0.1f, 0.2f, 0.3f, 0.4f));
            frame.EndFrame();

            FrameInterpreter interpreter;
            const DynamicArray<RenderOp> ops = interpreter.Interpret(frame);

            // 7 種 op:BEGIN / SET_CAMERA / DRAW_RENDERABLE / DRAW_MESH /
            // SET_VIEWPORT / DRAW_TEXT / END(PushTransform/BindMaterial 無獨立 op)。
            EXPECT_TRUE(ops.GetNElements() == 7, "7 個 op(前置 1-3 push/bind 合併、後置 6-7 draw/setview/text/end).", true);

            EXPECT_TRUE(ops[0].op == RenderOp::Op::BeginFrame, "op[0] = BeginFrame.", true);
            EXPECT_TRUE(ops[1].op == RenderOp::Op::SetCamera, "op[1] = SetCamera.", true);
            // #80:SetCamera op 攜帶值快照(取代 Camera* 指標)— pos/rot/AoV/planes 已快照。
            EXPECT_TRUE(ops[1].camera.position.x == 10.0f && ops[1].camera.position.y == 20.0f &&
                            ops[1].camera.position.z == 30.0f && ops[1].camera.rotation.y == 90.0f,
                        "SetCamera op 攜帶值快照(position/rotation 已快照,不再是指標).", true);
            const Frame::CameraPayload &activeCam = interpreter.GetActiveCamera();
            EXPECT_TRUE(activeCam.position.x == 10.0f && activeCam.rotation.y == 90.0f,
                        "interpreter 的 active camera 值快照更新.", false);
            EXPECT_TRUE(ops[2].op == RenderOp::Op::DrawRenderable && ops[2].pRenderable == &renderable,
                        "DrawRenderable op 帶 IRenderable 指標.", true);
            EXPECT_TRUE(ops[3].op == RenderOp::Op::DrawMesh && ops[3].meshId == 0x1234ULL,
                        "DrawMesh op 帶 meshId.", true);
            EXPECT_TRUE(ops[3].materialId == 0xABCULL, "DrawMesh op 解析 BindMaterial 的 materialId.", true);
            EXPECT_TRUE(ops[4].op == RenderOp::Op::SetViewport && ops[4].viewport.width == 800.0f &&
                            ops[4].viewport.height == 600.0f && ops[4].viewport.x == 12.5f && ops[4].viewport.y == 13.5f,
                        "SetViewport op 四欄原樣到達.", true);
            EXPECT_TRUE(ops[5].op == RenderOp::Op::DrawText && ops[5].fontId == 9 &&
                            ops[5].text == String(u"Hi") && ops[5].textSize == 14.5f &&
                            ops[5].textColor.R == 0.1f && ops[5].textColor.A == 0.4f,
                        "DrawText op payload 原樣到達.", true);
            EXPECT_TRUE(ops[6].op == RenderOp::Op::EndFrame, "op[6] = EndFrame.", true);
        }

        // ═══ 2. transform stack 契約(真實 interpreter 行為)════════════════
        TEST_MESSAGE("FrameInterpreter: transform stack contract (depth 64 / empty-pop / reset)");

        {
            // 2a. 65 push → stack 停在 64;第 65 個忽略(與原 Vulkan 行為一致),
            //     DrawMesh 取 stack top = 第 64 個 push 的 world。
            Frame frame;
            frame.BeginFrame();
            for (int i = 0; i < 65; i++)
                frame.PushTransform(Point3D(0, 0, static_cast<float>(i)), Point3D(), Point3D(1, 1, 1));
            frame.DrawMesh(1);
            frame.EndFrame();

            FrameInterpreter interpreter;
            const DynamicArray<RenderOp> ops = interpreter.Interpret(frame);
            EXPECT_TRUE(interpreter.GetStackDepth() == 64, "超過 64 的 push 被忽略(深度停在 64).", true);
            // stack top = 第 64 個 push(i=63):world = translate(0,0,63)
            const glm::mat4 expectedTop = RendererMath::BuildWorldMatrix(
                Point3D(0, 0, 63), Point3D(), Point3D(1, 1, 1));
            // DrawMesh op 是第 history:push(無 op) + drawmesh = 1 個 op(在 Begin/End 之間)
            EXPECT_TRUE(ops[1].op == RenderOp::Op::DrawMesh, "DrawMesh op 存在.", true);
            EXPECT_TRUE(MatEqual(ops[1].world, expectedTop), "DrawMesh world = 第 64 個 push 的 world(第 65 個被忽略).", true);
        }

        {
            // 2b. empty-pop no-op + 空 DrawMesh 上 idle:不崩潰、stack 深度 0。
            Frame frame;
            frame.BeginFrame();
            frame.PopTransform(); // 空 stack
            frame.DrawMesh(2);
            frame.EndFrame();

            FrameInterpreter interpreter;
            const DynamicArray<RenderOp> ops = interpreter.Interpret(frame);
            EXPECT_TRUE(interpreter.GetStackDepth() == 0, "empty-pop 後深度仍為 0(no-op).", true);
            EXPECT_TRUE(ops.GetNElements() == 3, "3 個 op(Begin/Mesh/End):push/pop 都不產 op.", true);
            EXPECT_TRUE(IsIdentity(ops[1].world), "空 stack 的 DrawMesh world = identity fallback.", true);
        }

        {
            // 2c. BeginFrame 重置 stack / material(跨幀不殘留)。
            // 單一 interpreter 連續 Interpret 兩幀:第一幀 push x2 + bind;
            // 第二幀只有 empty DrawMesh → world=identity、material=0。
            FrameInterpreter interpreter;

            Frame frameA;
            frameA.BeginFrame();
            frameA.PushTransform(Point3D(5, 0, 0), Point3D(), Point3D(1, 1, 1));
            frameA.PushTransform(Point3D(0, 5, 0), Point3D(), Point3D(1, 1, 1));
            frameA.BindMaterial(0x777);
            frameA.EndFrame();
            interpreter.Interpret(frameA);
            EXPECT_TRUE(interpreter.GetStackDepth() == 2, "第一幀 stack 深度 2.", true);

            Frame frameB;
            frameB.BeginFrame();
            frameB.DrawMesh(3);
            frameB.EndFrame();
            const DynamicArray<RenderOp> opsB = interpreter.Interpret(frameB);
            EXPECT_TRUE(interpreter.GetStackDepth() == 0, "第二幀 BeginFrame 重置 stack(深度 0).", true);
            EXPECT_TRUE(IsIdentity(opsB[1].world), "重置後 DrawMesh world = identity(無殘留).", true);
            EXPECT_TRUE(opsB[1].materialId == 0, "重置後 material = 0(無殘留).", true);
        }

        // ═══ 3. world 解析:PushTransform 用 RendererMath 合成、DrawText 同構 ══
        TEST_MESSAGE("FrameInterpreter: world resolution (PushTransform → RendererMath, DrawText same)");

        {
            Frame frame;
            frame.BeginFrame();
            // TRS 組合與 RendererMath 一致(translate→rotate X/Y/Z→scale)
            frame.PushTransform(Point3D(1.5f, -2.0f, 3.25f), Point3D(30, 45, 60), Point3D(2, 3, 4));
            frame.DrawText(9, String(u"x"), 12.0f, Color(1, 1, 1, 1));
            frame.EndFrame();

            FrameInterpreter interpreter;
            const DynamicArray<RenderOp> ops = interpreter.Interpret(frame);
            const glm::mat4 expected = RendererMath::BuildWorldMatrix(
                Point3D(1.5f, -2.0f, 3.25f), Point3D(30, 45, 60), Point3D(2, 3, 4));
            EXPECT_TRUE(ops[1].op == RenderOp::Op::DrawText, "DrawText op 存在.", true);
            EXPECT_TRUE(MatEqual(ops[1].world, expected), "DrawText 用 PushTransform 合成的 world.", true);
        }

        SUCCESS_MESSAGE("FrameInterpreter");
        return true;
    }
};

} // namespace frameinterpretertest

#endif // FRAME_INTERPRETER_TEST_HPP