#ifndef EDITOR_SESSION_TEST_HPP
#define EDITOR_SESSION_TEST_HPP

#include "Display/GUI/EditorSession.hpp"
#include "Test.hpp"

// #72 ADR-0001 D4:EditorSession 是 window-agnostic 編輯器模型
// (pointer set + Selection + undo/redo + pane registry),完全不依賴 GLFW/Vulkan
// → 可 headless 測試。
namespace editorsessiontest
{

// Mock renderer asset registrar:記錄 RegisterMaterial 呼叫,供 EditorSession texture 集成測試驗證。
class MockRegistrar : public IRendererAssetRegistrar
{
  public:
    bool registerCalled = false;
    uint64_t lastMaterialId = 0;
    int lastWidth = 0;
    int lastHeight = 0;
    bool lastHadRawRGBA = false;

    bool RegisterMeshGeometry(uint64_t meshId, const RenderInfo &info) override { return false; }

    bool RegisterMaterial(uint64_t materialId, const MaterialSource &source) override
    {
        registerCalled = true;
        lastMaterialId = materialId;
        lastWidth = source.width;
        lastHeight = source.height;
        lastHadRawRGBA = (source.pRawRGBA != nullptr);
        return true;
    }
};

// Test-local command: 對一個 float 做 set(記錄舊值)。
class FloatSetCommand : public EditorCommand
{
  public:
    FloatSetCommand(float *pTarget, float newValue, const String &label)
        : mpTarget(pTarget), mOldValue(*pTarget), mNewValue(newValue), mLabel(label)
    {
    }

    void Apply() override { *mpTarget = mNewValue; }
    void Revert() override { *mpTarget = mOldValue; }
    String Description() const override { return mLabel; }

  private:
    float *mpTarget;
    float mOldValue;
    float mNewValue;
    String mLabel;
};

class EditorSessionTest : public Test
{
  public:
    EditorSessionTest() : Test("EditorSession")
    {
    }

    bool Run() noexcept override
    {
        // ---- AC1:Selection 單一真相來源 ----
        {
            EditorSession session;
            EXPECT_TRUE(!session.GetSelection().hasSelection, "初始無選取.", true);
            session.Select(7);
            EXPECT_TRUE(session.GetSelection().entityIndex == 7, "Select 更新 entityIndex.", true);
            EXPECT_TRUE(session.GetSelection().hasSelection, "Select 設 hasSelection=true.", true);
            session.ClearSelection();
            EXPECT_TRUE(!session.GetSelection().hasSelection, "ClearSelection 清空選取.", true);
        }

        // ---- AC2:Undo/Redo 堆疊 ----
        {
            EditorSession session;
            UndoStack &stack = session.GetUndoStack();

            float value = 0.f;
            stack.Push(new FloatSetCommand(&value, 1.f, String(u"set 1")));
            stack.Push(new FloatSetCommand(&value, 2.f, String(u"set 2")));
            EXPECT_TRUE(stack.GetUndoCount() == 2, "push 兩個 command 後 undoCount==2.", true);

            EXPECT_TRUE(stack.Undo(), "undo 可執行.", true);
            EXPECT_TRUE(value == 1.f, "undo 後 value 回到 1.", true);
            EXPECT_TRUE(stack.GetRedoCount() == 1, "undo 後 redoCount==1.", true);

            EXPECT_TRUE(stack.Redo(), "redo 可執行.", true);
            EXPECT_TRUE(value == 2.f, "redo 後 value 回到 2.", true);
            EXPECT_TRUE(stack.GetUndoCount() == 2 && stack.GetRedoCount() == 0,
                        "redo 後 undo/redo count 正確.", true);
        }

        // ---- AC3:push 新的 command 清空 redo ----
        {
            EditorSession session;
            UndoStack &stack = session.GetUndoStack();
            float value = 0.f;
            stack.Push(new FloatSetCommand(&value, 1.f, String(u"a")));
            stack.Undo();
            EXPECT_TRUE(stack.GetRedoCount() == 1, "undo 後 redoCount==1.", true);
            stack.Push(new FloatSetCommand(&value, 5.f, String(u"b")));
            EXPECT_TRUE(stack.GetRedoCount() == 0, "push 新 command 清空 redo.", true);
        }

        // ---- AC4:Reset 清空 selection 與 undo stack ----
        {
            EditorSession session;
            session.Select(3);
            float value = 0.f;
            session.GetUndoStack().Push(new FloatSetCommand(&value, 1.f, String(u"x")));
            session.Reset();
            EXPECT_TRUE(!session.GetSelection().hasSelection, "Reset 後無選取.", true);
            EXPECT_TRUE(session.GetUndoStack().GetUndoCount() == 0, "Reset 後 undo stack 清空.", true);
        }

        // ---- AC5:D4 pointer set(注入 + 讀回)----
        {
            SceneSystem scene;
            CameraController cam;
            SimRecorder rec;
            EditorSession session(&scene, &cam, &rec);
            EXPECT_TRUE(session.GetSceneSystem() == &scene, "GetSceneSystem 讀回 scene 指標.", true);
            EXPECT_TRUE(session.GetCameraController() == &cam, "GetCameraController 讀回 camera 指標.", true);
            EXPECT_TRUE(session.GetSimRecorder() == &rec, "GetSimRecorder 讀回 recorder 指標.", true);
            // 未注入 = nullptr(default ctor)
            EditorSession empty;
            EXPECT_TRUE(empty.GetSceneSystem() == nullptr, "default ctor scene=nullptr.", true);
            EXPECT_TRUE(empty.GetCameraController() == nullptr, "default ctor camera=nullptr.", true);
            EXPECT_TRUE(empty.GetSimRecorder() == nullptr, "default ctor recorder=nullptr.", true);
        }

        // ---- AC6:editor 快捷鍵(Ctrl+Z/Y)走 session,不牽 Window ----
        {
            float value = 0.f;
            EditorSession session;
            session.GetUndoStack().Push(new FloatSetCommand(&value, 5.f, String(u"set")));

            EXPECT_TRUE(session.HandleEditorShortcut(true, true, false), "Ctrl+Z 被消費.", true);
            EXPECT_TRUE(value == 0.f, "Ctrl+Z 觸發 undo.", true);
            EXPECT_TRUE(session.HandleEditorShortcut(true, false, true), "Ctrl+Y 被消費.", true);
            EXPECT_TRUE(value == 5.f, "Ctrl+Y 觸發 redo.", true);

            EXPECT_TRUE(!session.HandleEditorShortcut(false, false, false), "無按鍵不消費.", true);
            EXPECT_TRUE(!session.HandleEditorShortcut(true, false, false), "只有 Ctrl 不消費.", true);
            EXPECT_TRUE(!session.HandleEditorShortcut(false, true, false), "只有 Z 不消費.", true);
        }

        // ---- AC7:D3/D4 pane registry(register / GetPane / idempotent / collapse)----
        {
            EditorSession session;
            PanelPane *pConsole = session.RegisterPane(
                SharedPtr<PanelPane>::Construct(String(u"Console"), Point2D(1000, 800), Border(),
                                                PanelDock::RightDock));
            EXPECT_TRUE(pConsole, "RegisterPane 回傳非空.", true);
            EXPECT_TRUE(session.GetPane(String(u"Console")) == pConsole, "GetPane 按 title 取回同一 pane.", true);
            EXPECT_TRUE(session.GetPane(String(u"MissingPane")) == nullptr, "不存在 title 回傳 nullptr.", true);
            EXPECT_TRUE(session.GetPanes().GetNElements() == 1, "registry 有一個 pane.", true);

            // 重複 title → 回傳既有 pane,不重複註冊
            PanelPane *pAgain = session.RegisterPane(
                SharedPtr<PanelPane>::Construct(String(u"Console"), Point2D(1000, 800), Border(),
                                                PanelDock::LeftDock));
            EXPECT_TRUE(pAgain == pConsole, "重複 title idempotent(回傳既有 pane).", true);
            EXPECT_TRUE(pAgain->GetDock() == PanelDock::RightDock, "idempotent 不改 dock.", true);
            EXPECT_TRUE(session.GetPanes().GetNElements() == 1, "idempotent 不增加 registry.", true);

            // dock / collapse 可改
            session.SetPaneCollapsed(String(u"Console"), true);
            EXPECT_TRUE(pConsole->IsCollapsed(), "SetPaneCollapsed 生效.", true);
            pConsole->SetDock(PanelDock::LeftDock);
            EXPECT_TRUE(pConsole->GetDock() == PanelDock::LeftDock, "SetDock 生效.", true);

            // 重新設定 collapse false
            session.SetPaneCollapsed(String(u"Console"), false);
            EXPECT_TRUE(!pConsole->IsCollapsed(), "SetPaneCollapsed(false) 回復.", true);
        }

        // ---- AC8:EditorSession + TextureAssetView 集成(load → decode → RegisterMaterial)----
        {
            JobSystem jobs(2);
            AssetManager manager(jobs);
            MockRegistrar mockReg;
            EditorSession session(nullptr, nullptr, nullptr, &manager, &mockReg);

            const char *path = TestTempFilePath("chia_editor_tex.ppm");
            WriteTestPpm(path, 2, 1, 10, 20, 30); // RGB(10,20,30) × 2 pixels

            uint64_t pathHash = session.LoadTexture(path);
            EXPECT_TRUE(pathHash != 0, "LoadTexture 回傳非零 pathHash", true);

            jobs.WaitForIdle();
            session.DispatchAssetEvents();

            EXPECT_TRUE(mockReg.registerCalled, "RegisterMaterial 被呼叫", true);
            EXPECT_TRUE(mockReg.lastHadRawRGBA, "RegisterMaterial 收到 pRawRGBA", true);
            EXPECT_TRUE(mockReg.lastWidth == 2 && mockReg.lastHeight == 1, "解碼尺寸 2x1", true);

            uint64_t materialId = session.GetMaterialId(path);
            EXPECT_TRUE(materialId != 0, "GetMaterialId 回傳非零", true);
            EXPECT_TRUE(materialId == mockReg.lastMaterialId, "GetMaterialId == RegisterMaterial 的 materialId", true);
            EXPECT_TRUE(session.IsTextureLoaded(path), "IsTextureLoaded 回傳 true", true);

            // 重複 LoadTexture(同一 path) → 依 key 去重,回傳相同 pathHash
            uint64_t pathHash2 = session.LoadTexture(path);
            EXPECT_TRUE(pathHash2 == pathHash, "重複 LoadTexture 回傳相同 pathHash", true);

            std::remove(path);
        }

        SUCCESS_MESSAGE("EditorSession: selection / undo-redo / pointer set / shortcuts / pane registry / texture integration 全部通過.");
        return true;
    }

  private:
    // 手工組裝最小 PPM P6(二進位 RGB,無壓縮,無跨平台差異)。
    static void WriteTestPpm(const char *path, int w, int h,
                             unsigned char r, unsigned char g, unsigned char b)
    {
        std::ofstream file(path, std::ios::binary);
        file << "P6\n" << w << " " << h << "\n255\n";
        for (int i = 0; i < w * h; ++i)
        {
            file.put(r); file.put(g); file.put(b);
        }
    }
};

} // namespace editorsessiontest

#endif // EDITOR_SESSION_TEST_HPP