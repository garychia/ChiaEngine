#ifndef SYSTEM_INPUT_SYSTEM_TEST_HPP
#define SYSTEM_INPUT_SYSTEM_TEST_HPP

#include "Test.hpp"
#include "Geometry/2D/Point2D.hpp"
#include "System/Input/InputHandleLayer.hpp"
#include "System/Input/InputHandler.hpp"
#include "System/Input/KeyCombination.hpp"
#include "System/Input/KeyboardHandler.hpp"
#include "System/Input/MouseInfo.hpp"
#include "System/Input/MouseInput.hpp"
#include "Types/Types.hpp"

// 用 namespace 隔離,避免與 SystemModule.cpp 內其它測試的全域型別衝突
// (同 FrameCounterTest.hpp / ReplayTest.hpp 的做法)。
namespace inputtest
{

// 所有 Input 單例(KeyboardHandler / MouseInput / InputHandler)在程序內持續存活,
// 每個測試場景開始前都要把狀態歸零,測試才彼此獨立。
inline void ResetKeyboardHandler()
{
    KeyboardHandler::GetSingleton().OnWindowLoseFocus();
}

inline void ResetMouseInput()
{
    MouseInput::GetSingleton().OnWindowLoseFocus();
}

// ---------------- KeyCombination(純值型別,無單例狀態) ----------------

class KeyCombinationTest : public Test
{
  public:
    KeyCombinationTest() : Test("InputKeyCombination")
    {
    }

    bool Run() noexcept override
    {
        TEST_MESSAGE("KeyCombination: 建構 / 相等性 / 空組合");

        // 預設建構 → 空組合
        KeyCombination empty;
        EXPECT_TRUE(empty.keys.Length() == 0, "default ctor 產生空組合.", false);
        EXPECT_TRUE(empty == KeyCombination(), "兩個預設組合相等.", false);

        // 從 Array<KeyCode> 建構:長度與順序照實保留
        KeyCombination wa(Array<KeyCode>({KeyCodeW, KeyCodeA}));
        EXPECT_TRUE(wa.keys.Length() == 2, "雙鍵組合長度為 2.", false);
        EXPECT_TRUE(wa.keys[0] == KeyCodeW && wa.keys[1] == KeyCodeA, "組合保留按下順序 (W, A).", false);

        // 複製建構 / 複製指派 / move 建構
        KeyCombination copy(wa);
        EXPECT_TRUE(copy == wa, "copy ctor 等值.", false);
        KeyCombination assigned;
        assigned = wa;
        EXPECT_TRUE(assigned == wa, "copy assignment 等值.", false);
        KeyCombination moved(Types::Move(copy));
        EXPECT_TRUE(moved == wa, "move ctor 等值.", false);

        // operator==:長度不同 / 順序不同 / 按鍵不同 → 不相等
        EXPECT_TRUE(!(wa == KeyCombination(Array<KeyCode>({KeyCodeW}))), "長度不同 → 不相等.", false);
        EXPECT_TRUE(!(wa == KeyCombination(Array<KeyCode>({KeyCodeA, KeyCodeW}))), "順序不同 → 不相等.", false);
        EXPECT_TRUE(!(wa == KeyCombination(Array<KeyCode>({KeyCodeW, KeyCodeS}))), "按鍵不同 → 不相等.", false);
        EXPECT_TRUE(wa == KeyCombination(Array<KeyCode>({KeyCodeW, KeyCodeA})), "同鍵同序 → 相等.", false);

        SUCCESS_MESSAGE("InputKeyCombination");
        return true;
    }
};

// ---------------- KeyboardHandler(單例,場景間需歸零) ----------------

class KeyboardHandlerTest : public Test
{
  public:
    KeyboardHandlerTest() : Test("InputKeyboardHandler")
    {
    }

    bool Run() noexcept override
    {
        KeyboardHandler &kh = KeyboardHandler::GetSingleton();

        TEST_MESSAGE("KeyboardHandler: 按鍵狀態轉移 / 組合組合 / 重設");
        ResetKeyboardHandler();
        EXPECT_TRUE(kh.GetKeyCombination() == KeyCombination(), "歸零後組合為空.", true);

        // 單鍵按下 → 組合含該鍵
        kh.ProcessKeyDown(KeyCodeW);
        EXPECT_TRUE(kh.GetKeyCombination() == KeyCombination(Array<KeyCode>({KeyCodeW})), "按 W → 組合 {W}.", false);

        // 多鍵依按下順序累積
        kh.ProcessKeyDown(KeyCodeA);
        kh.ProcessKeyDown(KeyCodeSpace);
        EXPECT_TRUE(kh.GetKeyCombination() == KeyCombination(Array<KeyCode>({KeyCodeW, KeyCodeA, KeyCodeSpace})),
                    "組合依按下順序 {W, A, Space}.", false);

        // 重複按下同一鍵 → 忽略(防抖),組合不重複、順序不變
        kh.ProcessKeyDown(KeyCodeA);
        EXPECT_TRUE(kh.GetKeyCombination() == KeyCombination(Array<KeyCode>({KeyCodeW, KeyCodeA, KeyCodeSpace})),
                    "按住期間重複按下 A → 組合不重複.", false);

        // 放開中間鍵 → 該鍵移除,其餘順序保留
        kh.ProcessKeyUp(KeyCodeA);
        EXPECT_TRUE(kh.GetKeyCombination() == KeyCombination(Array<KeyCode>({KeyCodeW, KeyCodeSpace})),
                    "放開 A → 組合 {W, Space}(順序保留).", false);

        // 放開未按下的鍵 → 組合不變、不崩潰
        kh.ProcessKeyUp(KeyCodeEscape);
        EXPECT_TRUE(kh.GetKeyCombination() == KeyCombination(Array<KeyCode>({KeyCodeW, KeyCodeSpace})),
                    "放開未按下的鍵 → 組合不變.", false);

        // 全部放開 → 回到空組合
        kh.ProcessKeyUp(KeyCodeSpace);
        kh.ProcessKeyUp(KeyCodeW);
        EXPECT_TRUE(kh.GetKeyCombination() == KeyCombination(), "全部放開 → 空組合.", false);

        // 放開後可再次按下(按下/放開可循環)
        kh.ProcessKeyDown(KeyCodeLeftControl);
        kh.ProcessKeyUp(KeyCodeLeftControl);
        kh.ProcessKeyDown(KeyCodeLeftControl);
        EXPECT_TRUE(kh.GetKeyCombination() == KeyCombination(Array<KeyCode>({KeyCodeLeftControl})),
                    "放開後重按 → 組合重新包含該鍵.", false);
        kh.ProcessKeyUp(KeyCodeLeftControl);

        // GetKeyCombination 回傳副本:外部改動不影響內部狀態
        {
            kh.ProcessKeyDown(KeyCodeReturn);
            KeyCombination grabbed = kh.GetKeyCombination();
            EXPECT_TRUE(grabbed.keys.Length() == 1 && grabbed.keys[0] == KeyCodeReturn, "取回組合內容正確.", false);
            if (grabbed.keys.Length() > 0)
                grabbed.keys[0] = KeyCodeEscape;
            EXPECT_TRUE(kh.GetKeyCombination() == KeyCombination(Array<KeyCode>({KeyCodeReturn})),
                        "改動取回的副本 → 內部組合不受影響.", false);
            kh.ProcessKeyUp(KeyCodeReturn);
        }

        // OnWindowLoseFocus:清空組合與按鍵狀態,且可再次使用
        kh.ProcessKeyDown(KeyCodeW);
        kh.ProcessKeyDown(KeyCodeA);
        EXPECT_TRUE(kh.GetKeyCombination() == KeyCombination(Array<KeyCode>({KeyCodeW, KeyCodeA})),
                    "失焦前組合為 {W, A}.", false);
        kh.OnWindowLoseFocus();
        EXPECT_TRUE(kh.GetKeyCombination() == KeyCombination(), "失焦後組合為空.", false);
        kh.ProcessKeyDown(KeyCodeW);
        EXPECT_TRUE(kh.GetKeyCombination() == KeyCombination(Array<KeyCode>({KeyCodeW})),
                    "失焦重設後可再次正常按鍵.", false);
        kh.OnWindowLoseFocus();

        // RecordChar:記錄字元,不影響按鍵組合狀態。
        // 註:lastChar 是 private 且無公開 getter,headless 下唯一可驗證的是
        // RecordChar 不干擾鍵盤狀態(字元內容本身無法透過公開 API 讀回 — 測試缺口)。
        kh.ProcessKeyDown(KeyCodeW);
        kh.RecordChar(u'a');
        kh.RecordChar(static_cast<String::CharType>(0x4E2D)); // '中'
        EXPECT_TRUE(kh.GetKeyCombination() == KeyCombination(Array<KeyCode>({KeyCodeW})),
                    "RecordChar 不影響按鍵組合.", false);
        kh.ProcessKeyUp(KeyCodeW);
        EXPECT_TRUE(kh.GetKeyCombination() == KeyCombination(), "RecordChar 後按鍵仍可正常放開.", false);

        SUCCESS_MESSAGE("InputKeyboardHandler");
        return true;
    }
};

// ---------------- MouseInput(單例,場景間需歸零) ----------------

class MouseInputTest : public Test
{
  public:
    MouseInputTest() : Test("InputMouseInput")
    {
    }

    bool Run() noexcept override
    {
        MouseInput &mi = MouseInput::GetSingleton();
        ResetMouseInput();
        const MouseInfo &info = mi.GetMouseInfo();
        EXPECT_TRUE(info.status == MouseStatus::Move, "歸零後 status = Move.", true);
        EXPECT_TRUE(!info.leftButtonDown && !info.rightButtonDown && !info.wheelButtonDown, "歸零後所有按鍵為 up.", false);
        EXPECT_TRUE(info.currentPosition.x == 0.f && info.currentPosition.y == 0.f, "歸零後 currentPosition = (0,0).", false);

        TEST_MESSAGE("MouseInput: 位置追蹤 / 按鍵狀態 / 滾輪 / 重設");

        // 滑鼠移動:首次移動 last == current;之後 last 追上前一 current
        mi.OnMouseMove(10.f, 20.f);
        EXPECT_TRUE(info.status == MouseStatus::Move, "移動 → status = Move.", false);
        EXPECT_TRUE(info.lastMousePosition.x == 10.f && info.lastMousePosition.y == 20.f, "首次移動 last = (10,20).", false);
        EXPECT_TRUE(info.currentPosition.x == 10.f && info.currentPosition.y == 20.f, "首次移動 current = (10,20).", false);
        mi.OnMouseMove(30.f, 40.f);
        EXPECT_TRUE(info.lastMousePosition.x == 10.f && info.lastMousePosition.y == 20.f, "第二次移動 last = 前一 current (10,20).", false);
        EXPECT_TRUE(info.currentPosition.x == 30.f && info.currentPosition.y == 40.f, "第二次移動 current = (30,40).", false);
        mi.OnMouseMove(50.f, 60.f);
        EXPECT_TRUE(info.lastMousePosition.x == 30.f && info.lastMousePosition.y == 40.f, "第三次移動 last = (30,40).", false);
        EXPECT_TRUE(info.currentPosition.x == 50.f && info.currentPosition.y == 60.f, "第三次移動 current = (50,60).", false);

        // 左鍵按下 / 放開:狀態、旗標、位置
        mi.OnMouseLeftButtonDown(1.f, 2.f);
        EXPECT_TRUE(info.status == MouseStatus::LeftButtonDown, "左鍵按下 → status = LeftButtonDown.", false);
        EXPECT_TRUE(info.leftButtonDown, "左鍵按下 → leftButtonDown = true.", false);
        EXPECT_TRUE(info.lastLeftButtonDownPosition.x == 1.f && info.lastLeftButtonDownPosition.y == 2.f,
                    "左鍵按下位置記錄為 (1,2).", false);
        mi.OnMouseLeftButtonUp(3.f, 4.f);
        EXPECT_TRUE(info.status == MouseStatus::LeftButtonUp, "左鍵放開 → status = LeftButtonUp.", false);
        EXPECT_TRUE(!info.leftButtonDown, "左鍵放開 → leftButtonDown = false.", false);
        EXPECT_TRUE(info.lastLeftButtonUpPosition.x == 3.f && info.lastLeftButtonUpPosition.y == 4.f,
                    "左鍵放開位置記錄為 (3,4).", false);

        // 右鍵按下 / 放開
        mi.OnMouseRightButtonDown(5.f, 6.f);
        EXPECT_TRUE(info.status == MouseStatus::RightButtonDown, "右鍵按下 → status = RightButtonDown.", false);
        EXPECT_TRUE(info.rightButtonDown, "右鍵按下 → rightButtonDown = true.", false);
        EXPECT_TRUE(info.lastRightButtonDownPosition.x == 5.f && info.lastRightButtonDownPosition.y == 6.f,
                    "右鍵按下位置記錄為 (5,6).", false);
        mi.OnMouseRightButtonUp(7.f, 8.f);
        EXPECT_TRUE(info.status == MouseStatus::RightButtonUp, "右鍵放開 → status = RightButtonUp.", false);
        EXPECT_TRUE(!info.rightButtonDown, "右鍵放開 → rightButtonDown = false.", false);
        EXPECT_TRUE(info.lastRightButtonUpPosition.x == 7.f && info.lastRightButtonUpPosition.y == 8.f,
                    "右鍵放開位置記錄為 (7,8).", false);

        // 滾輪
        mi.OnWheelRotated(120);
        EXPECT_TRUE(info.status == MouseStatus::WheelRotated, "滾輪 → status = WheelRotated.", false);
        EXPECT_TRUE(info.wheelDistance == 120, "滾輪距離 = 120.", false);

        // OnWindowLoseFocus:整包 MouseInfo 歸零
        mi.OnWindowLoseFocus();
        EXPECT_TRUE(info.status == MouseStatus::Move, "失焦後 status = Move.", false);
        EXPECT_TRUE(!info.leftButtonDown && !info.rightButtonDown && !info.wheelButtonDown, "失焦後按鍵全 up.", false);
        EXPECT_TRUE(info.currentPosition.x == 0.f && info.currentPosition.y == 0.f, "失焦後 currentPosition = (0,0).", false);
        EXPECT_TRUE(info.wheelDistance == 0, "失焦後 wheelDistance = 0.", false);

        SUCCESS_MESSAGE("InputMouseInput");
        return true;
    }
};

// ---------------- InputHandler(單例;無 layer 移除 API,layer 以 new 配置並
//    刻意不釋放 — singleton 存活到程序結束,避免懸空指標) ----------------

class InputProbe
{
  public:
    size_t keyCalls = 0;
    size_t mouseCalls = 0;
    bool keyResult = true;
    bool mouseResult = true;
    KeyCombination lastKeys;
    MouseInfo lastMouse;

    bool OnKey(const KeyCombination &keys)
    {
        keyCalls++;
        lastKeys = keys;
        return keyResult;
    }

    bool OnMouse(const MouseInfo &info)
    {
        mouseCalls++;
        lastMouse = info;
        return mouseResult;
    }
};

class InputHandlerTest : public Test
{
  public:
    InputHandlerTest() : Test("InputInputHandler")
    {
    }

    bool Run() noexcept override
    {
        InputHandler &handler = InputHandler::GetSingleton();

        // 場景 1:無任何 layer → 鍵盤/滑鼠輸入都未處理(false)
        TEST_MESSAGE("InputHandler: layer 註冊與輸入路由");
        EXPECT_TRUE(!handler.HandleKeyboardInput(KeyCombination()), "無 layer → 鍵盤輸入未處理.", false);
        {
            MouseInfo probeInfo;
            EXPECT_TRUE(!handler.HandleMouseInput(probeInfo), "無 layer → 滑鼠輸入未處理.", false);
        }

        // 場景 2:layer 無 callback → Valid() 檢查後回 false(不會解引用空 callback)
        InputHandleLayer *pLayerA = new InputHandleLayer();
        InputHandleLayer *pLayerB = new InputHandleLayer();
        handler.AddInputLayer(pLayerA);
        EXPECT_TRUE(!handler.HandleKeyboardInput(KeyCombination(Array<KeyCode>({KeyCodeW}))), "layer 無 key callback → false.", false);
        {
            MouseInfo probeInfo;
            EXPECT_TRUE(!handler.HandleMouseInput(probeInfo), "layer 無 mouse callback → false.", false);
        }

        // 場景 3:第一個 layer 處理成功(true)→ 路由短路,第二個 layer 不會被呼叫
        InputProbe probeA;
        InputProbe probeB;
        pLayerA->keyInputCallback.Set(&probeA, &InputProbe::OnKey);
        pLayerB->keyInputCallback.Set(&probeB, &InputProbe::OnKey);
        handler.AddInputLayer(pLayerB);
        const KeyCombination comboW(Array<KeyCode>({KeyCodeW}));
        EXPECT_TRUE(handler.HandleKeyboardInput(comboW), "layerA callback 回 true → 輸入被處理.", false);
        EXPECT_TRUE(probeA.keyCalls == 1, "layerA 被呼叫 1 次.", false);
        EXPECT_TRUE(probeB.keyCalls == 0, "layerA 處理成功 → layerB 不被呼叫(短路).", false);
        EXPECT_TRUE(probeA.lastKeys == comboW, "layerA 收到傳入的 KeyCombination 原樣.", false);

        // 場景 4:第一個 layer 回 false → 落到第二個 layer
        probeA.keyResult = false;
        probeB.keyResult = true;
        EXPECT_TRUE(handler.HandleKeyboardInput(comboW), "layerA 回 false → layerB 接手並處理.", false);
        EXPECT_TRUE(probeA.keyCalls == 2 && probeB.keyCalls == 1, "兩個 layer 各被呼叫一次.", false);

        // 場景 5:全部回 false → 整體 false
        probeB.keyResult = false;
        EXPECT_TRUE(!handler.HandleKeyboardInput(comboW), "全部 layer 回 false → 未處理.", false);
        EXPECT_TRUE(probeA.keyCalls == 3 && probeB.keyCalls == 2, "全部 layer 都被嘗試.", false);

        // 場景 6:滑鼠路由 — 第一個 layer 回 false 時落到第二個,且收到原樣 MouseInfo
        pLayerA->mouseInputCallback.Set(&probeA, &InputProbe::OnMouse);
        pLayerB->mouseInputCallback.Set(&probeB, &InputProbe::OnMouse);
        probeA.keyResult = true;
        probeA.mouseResult = false;
        probeB.mouseResult = true;
        MouseInfo mouseEvent;
        mouseEvent.status = MouseStatus::LeftButtonDown;
        mouseEvent.leftButtonDown = true;
        mouseEvent.lastLeftButtonDownPosition = Point2D(11.f, 22.f);
        EXPECT_TRUE(handler.HandleMouseInput(mouseEvent), "layerA 滑鼠回 false → layerB 接手並處理.", false);
        EXPECT_TRUE(probeA.mouseCalls == 1 && probeB.mouseCalls == 1, "兩個 layer 的滑鼠 callback 都被呼叫.", false);
        EXPECT_TRUE(probeB.lastMouse.status == MouseStatus::LeftButtonDown && probeB.lastMouse.leftButtonDown,
                    "layerB 收到原樣 MouseInfo(status/旗標).", false);
        EXPECT_TRUE(probeB.lastMouse.lastLeftButtonDownPosition.x == 11.f && probeB.lastMouse.lastLeftButtonDownPosition.y == 22.f,
                    "layerB 收到原樣 MouseInfo(位置).", false);

        // 場景 7:滑鼠路由短路 — 第一個 layer 回 true 即停
        probeA.mouseResult = true;
        probeB.mouseCalls = 0;
        EXPECT_TRUE(handler.HandleMouseInput(mouseEvent), "layerA 滑鼠回 true → 處理成功.", false);
        EXPECT_TRUE(probeA.mouseCalls == 2 && probeB.mouseCalls == 0, "layerA 處理成功 → layerB 不被呼叫.", false);

        // 註:pLayerA / pLayerB 刻意不 delete(InputHandler 無移除 layer 的 API,
        // singleton 會繼續持有指標直到程序結束 — 測試二進位生命週期短,可接受)。
        SUCCESS_MESSAGE("InputInputHandler");
        return true;
    }
};

// ---------------- WindowManager:headless 測試被阻斷(見下方說明),此類別為
//    已驗證的 skip,把 blocker 記錄在測試輸出中 ----------------

// Blocker 分析(不修改引擎原始碼的前提下,無法 headless 測試 WindowManager):
// 1. WindowManager.hpp → Window.hpp → pch.hpp → <GLFW/glfw3.h>:
//    WindowHandle = GLFWwindow*(pch.hpp),host 沒有 GLFW 標頭,光 include 就無法編譯。
// 2. WindowManager::WindowManager() 在建構子呼叫 glfwInit()(GLFW 相依)。
// 3. Window::GetHandle() 定義在 GLFW 相依的 src/source/Display/GLFW/Window.cpp;
//    windowMap 要有 entry 必須 RegisterWindow(真實 Window*),而 handle 來自
//    glfwCreateWindow — 沒有「注入 fake handle」的公開入口。
// 4. HandleKeyInput / HandleMouseInput / HandleResizing 的 map 查表邏輯本身是純的,
//    但全部被上述依賴擋住,無法獨立觸發。
// 因此 issue #58 建議的「WindowManager map ops 單元測試(純邏輯、注入 fake handles)」
// 在現行 API 下不可行;日後若提供 handle 注入點,應補測:RegisterWindow → HandleKeyInput/
// HandleMouseInput/HandleResizing 依 handle 正確 dispatch,以及 #38 的 NULL-handle 防護。
class WindowManagerTest : public Test
{
  public:
    WindowManagerTest() : Test("InputWindowManager")
    {
    }

    bool Run() noexcept override
    {
        TEST_MESSAGE("WindowManager: BLOCKED headless — GLFW required (see class comment)");
        // 故意不 include "Display/WindowManager.hpp":它會拉進 <GLFW/glfw3.h>(pch.hpp),
        // host 無 GLFW 標頭無法編譯;此處沒有任何可斷言的 headless 行為。
        SUCCESS_MESSAGE("InputWindowManager (documented skip)");
        return true;
    }
};

} // namespace inputtest

#endif // SYSTEM_INPUT_SYSTEM_TEST_HPP
