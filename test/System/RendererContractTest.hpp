#ifndef SYSTEM_RENDERER_CONTRACT_TEST_HPP
#define SYSTEM_RENDERER_CONTRACT_TEST_HPP

#include "Test.hpp"
#include "Data/Pointers.hpp"
#include "Display/Color.hpp"
#include "Display/IFrameExecutor.hpp" // Frame.hpp + 執行器契約(不拉 GLFW/Vulkan)
#include "Display/Camera.hpp"
#include "Display/IRenderable.hpp"
#include "Display/GUI/GUILayout.hpp"
#include "Display/GUI/GUILayer.hpp"
#include "Display/GUI/IGUI.hpp"
#include "Geometry/2D/Point2D.hpp"
#include "Geometry/3D/Point3D.hpp"
#include "Types/Types.hpp"

// RendererContractTest(issue #57 — renderer layer 的首個自動化測試):
// Frame(命令流) + IFrameExecutor(渲染器契約)是 headless 可測的 renderer 層表面;
// 真正的 GPU 後端(VulkanRenderer)不可無頭測試 → 見下方 VulkanRendererTest 的
// 4 點 blocker 分析(WindowManager precedent,issue #58)。
//
// 覆蓋範圍(與 #47 的 FrameSerializationTest 分工:不重複值命令 serialize 確定性,
// 而是測「executor 端」契約 + #47 沒測的 legacy 指標命令 + transform stack 契約):
//   1. executor dispatch 契約:MockExecutor 記錄每個 Execute(frame) 收到的命令
//      (命令列舉 + 酬載快照);Initialize 生命週期;OnWindowResized(0,0) no-op-safe。
//   2. payload fidelity:每個值命令的酬載從 Frame 錄製到 executor 端逐欄一致,
//      含 edge case:PushTransform 預設值 / DrawText 空字串 / 零視口。
//   3. legacy 指標命令(SetCamera/DrawRenderable/DrawGUILayout):命令類型正確、
//      Serialize 印語意內容(見 Frame::Serialize 註解)、round-trip 後指標歸 null
//      而命令數/順序保留(實際行為,誠實測試)。
//   4. transform stack 契約:不平衡 Push、空 stack Pop 安全、65 個 Push 全部錄製
//      (深度上限是 executor 的職責 — mock 實作 64 上限並驗證第 65 個被忽略,
//      與真實 Vulkan executor 的行為一致,見 VulkanRenderer.cpp L1134-1140)。
//
// 全檔包在 namespace renderercontracttest 內:所有 test header 編譯進同一個 TU,
// 裸 using namespace 仍會造成全域 lookup 碰撞(SystemModule.cpp 的 pitfall)。
namespace renderercontracttest
{

// 一次 Execute 中 executor 收到的單一命令:命令列舉 + 酬載快照。
// legacy 指標命令同時記錄 executor 實際看到的指標(round-trip 後應為 nullptr)。
struct ExecutedCommand
{
    Frame::Command command;
    Frame::TransformPayload transform;
    uint64_t meshId;
    uint64_t materialId;
    Frame::ViewportPayload viewport;
    uint64_t fontId;
    String text;
    float textSize;
    Color textColor;
    const Camera *pCamera;
    const IRenderable *pRenderable;
    const GUILayout *pLayout;

    ExecutedCommand() : command(Frame::Command::BeginFrame), transform(), meshId(0), materialId(0),
                        viewport(), fontId(0), text(), textSize(0), textColor(),
                        pCamera(nullptr), pRenderable(nullptr), pLayout(nullptr)
    {
    }
};


// ── MockExecutor:IFrameExecutor 的測試替身 ────────────────────────────────
// 記錄每一次 Execute(frame) 收到的每一條命令(命令列舉 + 酬載快照),並實作
// executor 端 transform stack 契約(深度上限 64)。行為刻意對齊真實 Vulkan
// executor(從 src/source/Display/Vulkan/VulkanRenderer.cpp 驗證):
//   - BeginFrame 重置 stack(VulkanRenderer.cpp L1089);
//   - PushTransform 超過 64 即忽略、不 append(VulkanRenderer.cpp L1134-1135);
//   - PopTransform 空 stack 為 no-op(VulkanRenderer.cpp L1139-1140)。
class MockExecutor : public IFrameExecutor
{
  public:
    static constexpr size_t MaxStackDepth = 64; // 與 VulkanRenderer::MaxTransformStackDepth 相同

    // 生命週期
    bool initialized = false;   // Initialize() 後為 true
    size_t executeCount = 0;    // Execute 成功次數
    size_t executeBeforeInit = 0; // 未 Initialize 就 Execute 的次數(契約違反)
    size_t resizeCalls = 0;     // OnWindowResized 呼叫次數
    long lastResizeWidth = -1;
    long lastResizeHeight = -1;

    // 命令流記錄(executor 實際看到的)
    DynamicArray<ExecutedCommand> records;

    // executor 端 transform stack(測試 4)
    DynamicArray<Frame::TransformPayload> stack;
    size_t stackRejects = 0; // 超過 64 被忽略的 PushTransform 數
    size_t popOnEmpty = 0;   // 空 stack 上的 PopTransform 數(no-op)

    bool Initialize(const Window * /*pWindow*/) override
    {
        initialized = true;
        return true; // mock 不觸碰 Window(真實後端需 GLFW window — 見 VulkanRendererTest blocker)
    }

    bool Execute(const Frame &frame) override
    {
        if (!initialized)
        {
            executeBeforeInit++;
            return false; // 生命週期契約:Initialize 之前 Execute 不合法
        }
        executeCount++;
        for (size_t i = 0; i < frame.GetNumCommands(); i++)
        {
            const Frame::CommandData &cmd = frame.GetCommand(i);
            ExecutedCommand rec;
            rec.command = cmd.command;
            rec.transform = cmd.transform;
            rec.meshId = cmd.meshId;
            rec.materialId = cmd.materialId;
            rec.viewport = cmd.viewport;
            rec.fontId = cmd.fontId;
            rec.text = cmd.text;
            rec.textSize = cmd.textSize;
            rec.textColor = cmd.textColor;
            rec.pCamera = cmd.pCamera;
            rec.pRenderable = cmd.pRenderable;
            rec.pLayout = cmd.pLayout;
            records.Append(Types::Move(rec));

            switch (cmd.command)
            {
                case Frame::Command::BeginFrame:
                    stack.RemoveAll(); // 幀起點重置(與 VulkanRenderer.cpp L1089 相同)
                    break;
                case Frame::Command::PushTransform:
                    if (stack.GetNElements() < MaxStackDepth)
                        stack.Append(cmd.transform);
                    else
                        stackRejects++; // 第 65 個以後忽略(與 VulkanRenderer.cpp L1134 相同)
                    break;
                case Frame::Command::PopTransform:
                    if (!stack.IsEmpty())
                        stack.RemoveLast();
                    else
                        popOnEmpty++; // 空 stack 的 Pop 是 no-op(與 VulkanRenderer.cpp L1139 相同)
                    break;
                default:
                    break;
            }
        }
        return true;
    }

    void OnWindowResized(long newWidth, long newHeight) override
    {
        resizeCalls++;
        lastResizeWidth = newWidth;
        lastResizeHeight = newHeight;
    }
};

// ── 序列化 token helper:與 Frame::Serialize 使用同一組公開格式化 API ────────
// (Str<char16_t>::FromFloat(v, 6) / FromInt),用來驗證 legacy 命令的語意內容
// 確實寫進序列化字串(而不是指標位址)。
static String FloatToken(float value)
{
    return String(Str<char16_t>::FromFloat(value, 6));
}

static String IntToken(uint64_t value)
{
    return String(Str<char16_t>::FromInt(value));
}

static void AppendToken(String &line, const String &token)
{
    if (line.Length() > 0)
        line = line + String(u" ");
    line = line + token;
}

static void AppendFloatToken(String &line, float value)
{
    AppendToken(line, FloatToken(value));
}

static void AppendIntToken(String &line, uint64_t value)
{
    AppendToken(line, IntToken(value));
}

class RendererContractTest : public Test
{
  public:
    RendererContractTest() : Test("RendererContract")
    {
    }

    bool Run() noexcept override
    {
        // ═══ 1. Executor dispatch 契約 ═════════════════════════════════════
        TEST_MESSAGE("RendererContract: executor dispatch contract (lifecycle + all command types in order)");

        // 生命週期:Initialize 之前 Execute 不合法(mock 拒絕並記錄)
        {
            MockExecutor executor;
            Frame pre;
            pre.BeginFrame();
            pre.EndFrame();
            EXPECT_TRUE(!executor.Execute(pre), "Initialize 前 Execute → 拒絕(生命週期契約).", true);
            EXPECT_TRUE(executor.executeBeforeInit == 1, "mock 記錄 1 次違反生命週期的 Execute.", false);
            EXPECT_TRUE(executor.records.GetNElements() == 0, "被拒絕的 Execute 不產生命令記錄.", false);
        }

        // Initialize → Execute 一幀全命令:11 種命令類型依錄製順序 dispatch
        // (物件在作用域內保持存活 — Frame 只存指標,Serialization/deref 前不得懸空)
        {
            MockExecutor executor;
            Frame frame;
            SharedPtr<Camera> pCamera = SharedPtr<Camera>::Construct();
            IRenderable renderable;
            GUILayout layout;
            frame.BeginFrame();
            frame.SetCamera(WeakPtr<Camera>(pCamera));
            frame.DrawRenderable(renderable);
            frame.DrawGUILayout(layout);
            frame.PushTransform(Point3D(1, 2, 3), Point3D(0, 45, 0), Point3D(1, 2, 1));
            frame.BindMaterial(0xABCULL);
            frame.DrawMesh(0x1234ULL);
            frame.SetViewport(0, 0, 640, 480);
            frame.DrawText(5, String(u"Hi"), 12.0f, Color(1, 0, 0, 1));
            frame.PopTransform();
            frame.EndFrame();
            const size_t expectedCount = 11;

            EXPECT_TRUE(executor.Initialize(nullptr), "Initialize(nullptr) 成功(mock 不觸碰 Window).", true);
            EXPECT_TRUE(executor.initialized, "Initialize 後 initialized = true.", false);
            EXPECT_TRUE(executor.Execute(frame), "Initialize 後 Execute 成功.", true);
            EXPECT_TRUE(executor.executeCount == 1, "Execute 恰好執行 1 次.", false);
            EXPECT_TRUE(executor.records.GetNElements() == expectedCount, "每條錄製命令都 dispatch 給 executor.", true);
            EXPECT_TRUE(frame.GetNumCommands() == expectedCount, "幀含 11 條命令.", true);

            const Frame::Command expectedOrder[11] = {
                Frame::Command::BeginFrame,   Frame::Command::SetCamera,    Frame::Command::DrawRenderable,
                Frame::Command::DrawGUILayout, Frame::Command::PushTransform, Frame::Command::BindMaterial,
                Frame::Command::DrawMesh,     Frame::Command::SetViewport,   Frame::Command::DrawText,
                Frame::Command::PopTransform, Frame::Command::EndFrame,
            };
            for (size_t i = 0; i < expectedCount; i++)
            {
                EXPECT_TRUE(executor.records[i].command == expectedOrder[i],
                            "命令類型依序 dispatch(順序與錄製一致).", true);
            }
        }

        // OnWindowResized(0,0) 是 no-op-safe(不崩潰、記錄呼叫)
        {
            MockExecutor executor;
            executor.Initialize(nullptr);
            executor.OnWindowResized(0, 0);
            EXPECT_TRUE(executor.resizeCalls == 1, "OnWindowResized 被呼叫 1 次.", false);
            EXPECT_TRUE(executor.lastResizeWidth == 0 && executor.lastResizeHeight == 0,
                        "OnWindowResized(0,0) 參數原樣記錄、不崩潰(no-op-safe).", true);
        }

        // ═══ 2. Payload fidelity ══════════════════════════════════════════
        TEST_MESSAGE("RendererContract: payload fidelity (value commands reach executor unchanged)");

        {
            Frame frame;
            frame.BeginFrame();
            frame.PushTransform(Point3D(1.5f, -2.5f, 3.25f), Point3D(10.0f, 20.0f, 30.0f), Point3D(2.0f, 3.0f, 4.0f));
            frame.BindMaterial(0xDEADBEEFCAFEULL);
            frame.DrawMesh(0x1122334455667788ULL);
            frame.SetViewport(12.5f, 13.5f, 800.0f, 600.0f);
            frame.DrawText(9, String(u"payload fidelity"), 14.5f, Color(0.1f, 0.2f, 0.3f, 0.4f));
            frame.EndFrame();

            MockExecutor executor;
            executor.Initialize(nullptr);
            EXPECT_TRUE(executor.Execute(frame), "Execute 成功.", true);

            // executor 端收到的酬載 == Frame 錄製的酬載
            const ExecutedCommand &push = executor.records[1];
            EXPECT_TRUE(push.command == Frame::Command::PushTransform, "命令 1 為 PushTransform.", true);
            EXPECT_TRUE(push.transform.position.x == 1.5f && push.transform.position.y == -2.5f && push.transform.position.z == 3.25f,
                        "PushTransform position 原樣到達 executor.", true);
            EXPECT_TRUE(push.transform.rotation.x == 10.0f && push.transform.rotation.y == 20.0f && push.transform.rotation.z == 30.0f,
                        "PushTransform rotation 原樣到達 executor.", true);
            EXPECT_TRUE(push.transform.scale.x == 2.0f && push.transform.scale.y == 3.0f && push.transform.scale.z == 4.0f,
                        "PushTransform scale 原樣到達 executor.", true);

            const ExecutedCommand &material = executor.records[2];
            EXPECT_TRUE(material.command == Frame::Command::BindMaterial && material.materialId == 0xDEADBEEFCAFEULL,
                        "BindMaterial materialId 原樣到達 executor.", true);

            const ExecutedCommand &mesh = executor.records[3];
            EXPECT_TRUE(mesh.command == Frame::Command::DrawMesh && mesh.meshId == 0x1122334455667788ULL,
                        "DrawMesh meshId 原樣到達 executor.", true);

            const ExecutedCommand &viewport = executor.records[4];
            EXPECT_TRUE(viewport.command == Frame::Command::SetViewport && viewport.viewport.x == 12.5f &&
                            viewport.viewport.y == 13.5f && viewport.viewport.width == 800.0f && viewport.viewport.height == 600.0f,
                        "SetViewport 四欄原樣到達 executor.", true);

            const ExecutedCommand &text = executor.records[5];
            EXPECT_TRUE(text.command == Frame::Command::DrawText && text.fontId == 9, "DrawText fontId 原樣到達.", true);
            EXPECT_TRUE(text.text == String(u"payload fidelity"), "DrawText text 原樣到達 executor.", true);
            EXPECT_TRUE(text.textSize == 14.5f, "DrawText textSize 原樣到達 executor.", true);
            EXPECT_TRUE(text.textColor.R == 0.1f && text.textColor.G == 0.2f && text.textColor.B == 0.3f && text.textColor.A == 0.4f,
                        "DrawText textColor 原樣到達 executor.", true);

            // 交叉驗證:Frame 端錄製的 payload 與 executor 端一致
            const Frame::CommandData &fc = frame.GetCommand(1);
            EXPECT_TRUE(fc.transform.position.x == push.transform.position.x &&
                            fc.transform.position.y == push.transform.position.y &&
                            fc.transform.position.z == push.transform.position.z,
                        "Frame 錄製與 executor 接收的 transform 一致.", true);
        }

        // edge cases:PushTransform 預設 ctor 值 / DrawText 空字串 / 零視口
        {
            Frame frame;
            frame.BeginFrame();
            frame.PushTransform(Frame::TransformPayload()); // 預設:pos(0,0,0) rot(0,0,0) scale(1,1,1)
            frame.DrawText(3, String(), 12.0f, Color(0.5f, 0.5f, 0.5f, 1));
            frame.SetViewport(0, 0, 0, 0);
            frame.EndFrame();

            MockExecutor executor;
            executor.Initialize(nullptr);
            EXPECT_TRUE(executor.Execute(frame), "Execute 成功(edge case 幀).", true);

            const ExecutedCommand &push = executor.records[1];
            EXPECT_TRUE(push.command == Frame::Command::PushTransform, "命令 1 為 PushTransform.", true);
            EXPECT_TRUE(push.transform.position.x == 0.0f && push.transform.position.y == 0.0f && push.transform.position.z == 0.0f,
                        "PushTransform 預設 position = (0,0,0) 到達 executor.", true);
            EXPECT_TRUE(push.transform.rotation.x == 0.0f && push.transform.rotation.y == 0.0f && push.transform.rotation.z == 0.0f,
                        "PushTransform 預設 rotation = (0,0,0) 到達 executor.", true);
            EXPECT_TRUE(push.transform.scale.x == 1.0f && push.transform.scale.y == 1.0f && push.transform.scale.z == 1.0f,
                        "PushTransform 預設 scale = (1,1,1) 到達 executor.", true);

            const ExecutedCommand &text = executor.records[2];
            EXPECT_TRUE(text.command == Frame::Command::DrawText && text.text.Length() == 0,
                        "DrawText 空字串原樣到達 executor(Length == 0).", true);
            EXPECT_TRUE(text.fontId == 3 && text.textSize == 12.0f, "DrawText 空字串其餘酬載不變.", true);

            const ExecutedCommand &viewport = executor.records[3];
            EXPECT_TRUE(viewport.command == Frame::Command::SetViewport && viewport.viewport.x == 0.0f &&
                            viewport.viewport.y == 0.0f && viewport.viewport.width == 0.0f && viewport.viewport.height == 0.0f,
                        "零視口(0,0,0,0)原樣到達 executor.", true);
        }

        // ═══ 3. Legacy 指標命令(SetCamera / DrawRenderable / DrawGUILayout)══
        TEST_MESSAGE("RendererContract: legacy pointer commands (stream + semantic Serialize + nulled round-trip)");

        {
            // 物件在作用域內保持存活:Frame 對 legacy 命令只存指標,Serialize 會
            // 解引用(印語意內容),指標不得懸空 — 同 FrameTest.hpp 的做法。
            Frame frame;
            SharedPtr<Camera> pCamera = SharedPtr<Camera>::Construct(Point3D(10, 20, 30), Point3D(0, 90, 0));
            pCamera->SetAngleOfView(45.0f);
            pCamera->SetDistanceToNearPlane(0.5f);
            pCamera->SetDistanceToFarPlane(200.0f);
            IRenderable renderable(Point3D(1, 2, 3), Point3D(0, 0, 90), Point3D(2, 2, 2), Color(1, 0, 0, 1));
            renderable.MarkLoaded(0xABCDULL); // identifier 0xABCD = 43981
            GUILayout layout;
            SharedPtr<GUILayer> pLayer = SharedPtr<GUILayer>::Construct(Point2D(800, 600), Border());
            pLayer->MarkLoaded(0x11); // 0x11 = 17
            SharedPtr<IGUI> pComponent = pLayer->AddComponent<IGUI>(Point2D(800, 600), Border());
            pComponent->MarkLoaded(0x22); // 0x22 = 34
            layout.AddLayer(pLayer);
            frame.BeginFrame();
            frame.SetCamera(WeakPtr<Camera>(pCamera));
            frame.DrawRenderable(renderable);
            frame.DrawGUILayout(layout);
            frame.EndFrame();

            // 3a.命令類型正確出現在命令流(executor 收到 live 指標)
            MockExecutor executor;
            executor.Initialize(nullptr);
            EXPECT_TRUE(executor.Execute(frame), "Execute 成功(legacy 幀).", true);
            EXPECT_TRUE(executor.records.GetNElements() == 5, "5 條命令(Begin/SetCamera/DrawRenderable/DrawGUILayout/End).", true);
            EXPECT_TRUE(executor.records[1].command == Frame::Command::SetCamera, "命令 1 為 SetCamera.", true);
            EXPECT_TRUE(executor.records[2].command == Frame::Command::DrawRenderable, "命令 2 為 DrawRenderable.", true);
            EXPECT_TRUE(executor.records[3].command == Frame::Command::DrawGUILayout, "命令 3 為 DrawGUILayout.", true);
            // executor 執行當下指標有效(round-trip 前)
            EXPECT_TRUE(executor.records[1].pCamera != nullptr, "SetCamera 執行當下帶有效 Camera 指標.", true);
            EXPECT_TRUE(executor.records[2].pRenderable != nullptr, "DrawRenderable 執行當下帶有效 IRenderable 指標.", true);
            EXPECT_TRUE(executor.records[3].pLayout != nullptr, "DrawGUILayout 執行當下帶有效 GUILayout 指標.", true);

            // 3b.Serialize 印 legacy 命令的「語意內容」而非指標(見 Frame::Serialize 註解)
            //    用 Frame 同一組格式化 API(FromFloat(v,6)/FromInt)重建期望字串。
            const String serialized = frame.Serialize();

            String camLine = String(u"SetCamera");
            AppendFloatToken(camLine, 10.0f); // pos
            AppendFloatToken(camLine, 20.0f);
            AppendFloatToken(camLine, 30.0f);
            AppendFloatToken(camLine, 0.0f); // rot
            AppendFloatToken(camLine, 90.0f);
            AppendFloatToken(camLine, 0.0f);
            AppendFloatToken(camLine, 45.0f); // AoV
            AppendFloatToken(camLine, 0.5f); // near
            AppendFloatToken(camLine, 200.0f); // far

            String drawLine = String(u"DrawRenderable");
            AppendIntToken(drawLine, 0xABCDULL); // identifier
            AppendFloatToken(drawLine, 1.0f); // pos
            AppendFloatToken(drawLine, 2.0f);
            AppendFloatToken(drawLine, 3.0f);
            AppendFloatToken(drawLine, 0.0f); // rot
            AppendFloatToken(drawLine, 0.0f);
            AppendFloatToken(drawLine, 90.0f);
            AppendFloatToken(drawLine, 2.0f); // scale
            AppendFloatToken(drawLine, 2.0f);
            AppendFloatToken(drawLine, 2.0f);

            String guiLine = String(u"DrawGUILayout");
            AppendIntToken(guiLine, 0x11ULL); // layer identifier
            AppendIntToken(guiLine, 0x22ULL); // component identifier

            String expected = String(u"BeginFrame") + String(u"\n") + camLine + String(u"\n") + drawLine +
                              String(u"\n") + guiLine + String(u"\n") + String(u"EndFrame");
            EXPECT_TRUE(serialized == expected, "legacy 命令的語意內容(TRS/AoV/planes/identifier)寫進序列化字串.", true);

            // 3c. round-trip:命令數/順序保留,pointer payload 依文件歸 null
            //    (Frame.hpp 註解:legacy 指標命令無法無頭重建 → 只記錄命令、null 指標;
            //     round-trip 驗收只針對新命令 — 實際行為與文件意圖一致,以下測實際行為。)
            Frame rt = Frame::Deserialize(serialized);
            EXPECT_TRUE(rt.GetNumCommands() == frame.GetNumCommands(), "round-trip 後命令數保留(5 條).", true);
            for (size_t i = 0; i < frame.GetNumCommands(); i++)
            {
                EXPECT_TRUE(rt.GetCommand(i).command == frame.GetCommand(i).command,
                            "round-trip 後命令順序保留.", true);
            }
            EXPECT_TRUE(rt.GetCommand(1).pCamera == nullptr, "SetCamera 指標 round-trip 後歸 null(無法 round-trip).", true);
            EXPECT_TRUE(rt.GetCommand(2).pRenderable == nullptr, "DrawRenderable 指標 round-trip 後歸 null.", true);
            EXPECT_TRUE(rt.GetCommand(3).pLayout == nullptr, "DrawGUILayout 指標 round-trip 後歸 null.", true);

            // 誠實記錄實際行為:重序列化時 legacy 酬載變成全零(命令名保留)。
            // 這是文件預期的取捨 — 「round-trip 驗收只針對新命令」。
            String expectedNulled = String(u"BeginFrame") + String(u"\n");
            String nullCam = String(u"SetCamera");
            for (int k = 0; k < 9; k++)
                AppendFloatToken(nullCam, 0.0f);
            expectedNulled = expectedNulled + nullCam + String(u"\n");
            String nullDraw = String(u"DrawRenderable");
            AppendIntToken(nullDraw, 0);
            for (int k = 0; k < 9; k++)
                AppendFloatToken(nullDraw, 0.0f);
            expectedNulled = expectedNulled + nullDraw + String(u"\n");
            expectedNulled = expectedNulled + String(u"DrawGUILayout") + String(u"\n");
            expectedNulled = expectedNulled + String(u"EndFrame");
            EXPECT_TRUE(rt.Serialize() == expectedNulled, "round-trip 後重序列化:legacy 酬載歸零、命令名保留(實際行為).", true);

            // 3d. executor 執行 deserialize 後的幀:legacy 命令以 null 指標 dispatch,
            //     不崩潰(mock 記錄 null 指標;Vulkan executor 對 null 有 if 防護)
            MockExecutor rtExecutor;
            rtExecutor.Initialize(nullptr);
            EXPECT_TRUE(rtExecutor.Execute(rt), "執行 round-trip 幀安全(legacy null 指標).", true);
            EXPECT_TRUE(rtExecutor.records[1].command == Frame::Command::SetCamera && rtExecutor.records[1].pCamera == nullptr,
                        "deserialize 幀的 SetCamera 以 null 指標到達 executor.", true);
            EXPECT_TRUE(rtExecutor.records[2].command == Frame::Command::DrawRenderable && rtExecutor.records[2].pRenderable == nullptr,
                        "deserialize 幀的 DrawRenderable 以 null 指標到達 executor.", true);
            EXPECT_TRUE(rtExecutor.records[3].command == Frame::Command::DrawGUILayout && rtExecutor.records[3].pLayout == nullptr,
                        "deserialize 幀的 DrawGUILayout 以 null 指標到達 executor.", true);
        }

        // ═══ 4. Transform stack 契約 ══════════════════════════════════════
        TEST_MESSAGE("RendererContract: transform stack contract (unbalanced push / pop-on-empty / 65 pushes)");

        // 4a.不平衡 PushTransform(無 Pop):Frame 端正確錄製、executor 端正常堆疊、不崩潰
        {
            Frame frame;
            frame.BeginFrame();
            frame.PushTransform(Point3D(1, 1, 1), Point3D(), Point3D(1, 1, 1));
            frame.PushTransform(Point3D(2, 2, 2), Point3D(), Point3D(1, 1, 1));
            frame.PushTransform(Point3D(3, 3, 3), Point3D(), Point3D(1, 1, 1));
            frame.EndFrame();
            EXPECT_TRUE(frame.GetNumCommands() == 5, "不平衡 Push 仍正確錄製(3 push + Begin/End).", true);

            MockExecutor executor;
            executor.Initialize(nullptr);
            EXPECT_TRUE(executor.Execute(frame), "不平衡 Push 幀執行不崩潰.", true);
            EXPECT_TRUE(executor.stack.GetNElements() == 3, "executor 端 stack 深度 = 3(無 Pop).", true);
            EXPECT_TRUE(executor.stackRejects == 0 && executor.popOnEmpty == 0, "無越界、無空 Pop.", false);
        }

        // 4b.空 stack 上的 PopTransform:安全 no-op(與 VulkanRenderer.cpp L1139 相同)
        {
            Frame frame;
            frame.BeginFrame();
            frame.PopTransform(); // 空 stack
            frame.EndFrame();

            MockExecutor executor;
            executor.Initialize(nullptr);
            EXPECT_TRUE(executor.Execute(frame), "空 stack Pop 幀執行不崩潰.", true);
            EXPECT_TRUE(executor.popOnEmpty == 1, "mock 記錄 1 次空 stack Pop(no-op).", true);
            EXPECT_TRUE(executor.stack.GetNElements() == 0, "空 stack Pop 後深度仍為 0.", false);
        }

        // 4c.65 個 PushTransform(超過文件上限 64):
        //     Frame 端全部錄製(發射端無上限)→ executor 端只保留 64、第 65 個被忽略。
        //     註:深度強制是 executor 的職責(IFrameExecutor.hpp「深度上限 64」);
        //     mock 實作與真實 Vulkan executor 一致 — VulkanRenderer.cpp L1134:
        //     if (transformStack.GetNElements() < MaxTransformStackDepth) Append(world);
        //     → 第 65 個 push 被靜默忽略(不是錯誤,是防 Sim bug 溢出的保護)。
        {
            Frame frame;
            frame.BeginFrame();
            for (int i = 0; i < 65; i++)
                frame.PushTransform(Frame::TransformPayload());
            frame.EndFrame();
            EXPECT_TRUE(frame.GetNumCommands() == 67, "Frame 端錄製全部 65 個 PushTransform(發射端無上限).", true);

            MockExecutor executor;
            executor.Initialize(nullptr);
            EXPECT_TRUE(executor.Execute(frame), "65 個 Push 幀執行不崩潰.", true);
            EXPECT_TRUE(executor.records.GetNElements() == 67, "executor 收到全部 67 條命令(含 65 push).", true);
            EXPECT_TRUE(executor.stack.GetNElements() == 64, "executor 端 stack 停在 64(深度上限強制在 executor).", true);
            EXPECT_TRUE(executor.stackRejects == 1, "第 65 個 Push 被忽略(與 Vulkan executor 行為一致).", true);
        }

        SUCCESS_MESSAGE("RendererContract");
        return true;
    }
};

// ═══ VulkanRenderer:headless 測試被阻斷(WindowManager precedent,issue #58)══
// Blocker 分析(不修改引擎原始碼的前提下,無法 headless 測試真正的 GPU 執行器):
// 1. VulkanRenderer.hpp → pch.hpp:VULKAN_ENABLED 下 include <GLFW/glfw3.h>
//    (GLFW_INCLUDE_VULKAN)+ Vulkan 標頭;host 沒有 GLFW/Vulkan 標頭,光 include
//    就無法編譯(與 WindowManager 同型 — WindowHandle = GLFWwindow*)。
// 2. VulkanRenderer 需要真實 GPU:Initialize/建構建立 VkInstance/VkDevice/
//    swapchain,都來自真實 GLFW window handle;無頭環境沒有 GPU/視窗,
//    沒有「注入 fake device」的公開入口。
// 3. Execute() 的 GPU 翻譯(vkCmdSetViewport/vkCmdSetScissor、RecordMeshDraw-
//    Commands、RecordTextDrawCommands、BeginFrame/EndFrame 的 acquire/present)
//    全部需要 live command buffer/device — 沒有可替換 fake command buffer
//    的注入點。
// 4. Execute() 內「純邏輯可測」的部分(transform stack 64 上限、空 stack Pop
//    no-op、materialId 記錄、null 指標防護)與 GPU 呼叫在同一 switch 內,
//    無法獨立觸發 — 本檔的 MockExecutor 已把其中可驗證的契約(深度 64、
//    第 65 個忽略、空 Pop no-op)依 VulkanRenderer.cpp L1089/1134/1139 的
//    實際行為 headless 測掉。
// 因此不寫 fake GPU 程式碼。日後若提供 device/command buffer 注入點,應補測:
// SetViewport → vkCmdSetViewport/vkCmdSetScissor 對映(含 0,0 視口)、
// DrawMesh/DrawText 空 stack 時的 identity fallback(L1149/1177)、
// BeginFrame/EndFrame 的 swapchain acquire/present 與 VUID counting、以及
// Initialize 需真實 Window 的生命週期。
class VulkanRendererTest : public Test
{
  public:
    VulkanRendererTest() : Test("RendererVulkanRenderer")
    {
    }

    bool Run() noexcept override
    {
        TEST_MESSAGE("RendererVulkanRenderer: BLOCKED headless — GPU/Vulkan required (see class comment)");
        // 故意不 include "Display/Vulkan/VulkanRenderer.hpp":它會拉進 pch.hpp →
        // <GLFW/glfw3.h> + Vulkan 標頭,host 無這些標頭無法編譯;
        // 此處沒有任何可斷言的 headless 行為(契約已由 MockExecutor 覆蓋)。
        SUCCESS_MESSAGE("RendererVulkanRenderer (documented skip)");
        return true;
    }
};

} // namespace renderercontracttest

#endif // SYSTEM_RENDERER_CONTRACT_TEST_HPP
