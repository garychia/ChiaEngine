# ChiaEngine v2 Architecture — "Kernel & Projection"

> 設計目標:讓 ChiaEngine 從「自製容器 + 多後端 renderer 的展示工具」,
> 進化成「**可無頭測試的遊戲引擎**」。核心創意:**模擬與呈現徹底分離**,
> **Frame(渲染命令流)是兩者之間唯一的貨幣**。

---

## 0. Design Principles(為什麼這樣設計)

1. **Simulation is deterministic, Presentation is a projection.**
   遊戲邏輯(World)完全不碰 GPU 型別、不讀牆鐘。同樣的輸入 → 同樣的模擬結果。
   這讓你的 TestMain 變成引擎的靈魂:整套遊戲邏輯可以在無 GPU 的 CI 上 100% 測試,
   而且是**可重播的**(replay = 內建除錯超能力)。

2. **Frame 是唯一的貨幣。** Sim 每個 tick 產出一份 `Frame`(渲染命令列表),
   renderer 只是 Frame 的執行器。三個後端(Vulkan/DX/GL)從「各寫一套大 API」
   變成「各寫一個小 executor」,後端差異被壓到最小。

3. **Dogfooding:引擎用自己寫的容器。** ECS 的儲存直接建在你的
   `Array / HashTable / DynamicArray / List` 上。Data 層不再只是單元測試驗證,
   而是被引擎本身上線壓力驗證 — 這是自製容器活下去的唯一方式。

4. **Everything is a Module, modules talk through the Bus.**
   你已經有 `Event<T(Args...)>`(HashTable + Callback 型別安全事件)。
   把整個引擎切成模組,模組之間**只透過型別事件溝通**,不直接互相呼叫。
   這是熱重載、獨立測試、解耦的基礎。

5. **The game is a module too.** ChiaApp / 未來的 editor 用與引擎相同的 API 建構 —
   第一個 dogfood 就是引擎自己。

---

## 1. 分層總覽

```
┌──────────────────────────────────────────────────────────┐
│  App Layer(組合根)                                        │
│  ChiaApp · Editor · 你的遊戲          ← 全都只是 modules  │
├──────────────────────────────────────────────────────────┤
│  View Layer(呈現 = 模擬的投影)                             │
│  Frame(命令流) · IFrameExecutor · GUI · 音訊 · 粒子        │
│  VulkanExecutor / DirectXExecutor / OpenGLExecutor        │
├──────────────────────────────────────────────────────────┤
│  Sim Layer(模擬 = 引擎的心臟,可無頭執行)                   │
│  World(ECS) · Systems · 物理 · 遊戲邏輯                   │
│  ⚠ 禁止:GPU 型別、牆鐘、非確定性 RNG                      │
├──────────────────────────────────────────────────────────┤
│  Foundation(地基,已存在,繼續長)                           │
│  Data 容器 · Math · Types · IO · Debug · 記憶體 Arena     │
├──────────────────────────────────────────────────────────┤
│  Platform(平台層)                                         │
│  Window(GLFW/Win32) · Input · 執行緒/Job                  │
└──────────────────────────────────────────────────────────┘
         ↕ 全部模組透過 Event Bus + EngineContext 溝通
```

分層鐵律:**Sim 不依賴 View**。View 依賴 Sim(讀狀態、訂閱事件)。
Foundation 誰都不依賴。App 把一切組起來。

---

## 2. 模組介面:一切皆 Module

```cpp
// System/Module/IModule.hpp (新)
class IModule
{
  public:
    virtual ~IModule() = default;

    // 掛載時:註冊服務、訂閱事件
    virtual void OnAttach(EngineContext &ctx) = 0;
    virtual void OnDetach(EngineContext &ctx) = 0;

    // 固定步進:模擬 tick,保證確定性(Sim 模組才實作)
    virtual void FixedUpdate(const FrameClock &clock) { (void)clock; }

    // 每幀更新:允許非確定性(View/Platform 模組用)
    virtual void Update(const FrameClock &clock) { (void)clock; }

    // 錄製渲染命令:把狀態投影成 Frame
    virtual void Render(Frame &frame) { (void)frame; }
};
```

- `EngineContext`:服務註冊表(`HashTable<String, IService *>` + 型別查詢)。
  模組不存全域狀態,狀態住在 EngineContext 裡,模組之間透過事件拿通知。
- 模組生命週期由 `Engine`(組合根)管理:`Attach → [FixedUpdate×N / Update / Render]* → Detach`。
- 訂閱用你現成的 `Event<T(Args...)>`:`ctx.GetBus().Subscribe<CollisionEvent>(this, &MyModule::OnCollision)`。

---

## 3. Sim Layer:World = 你自己的容器的 ECS

### 3.1 Entity = 世代編號 handle

```cpp
// System/World/Entity.hpp (新)
using Entity = u32;  // 高 8 bit 世代,低 24 bit 索引
```

### 3.2 EntityRegistry — sparse set 建在你的容器上

```cpp
class EntityRegistry
{
  private:
    DynamicArray<EntityData> dense;     // 你的 DynamicArray
    HashTable<u32, u32>     sparse;     // 你的 HashTable: entity index -> dense index
    DynamicArray<u32>       generations;

  public:
    Entity Create();
    void   Destroy(Entity e);
    bool   Alive(Entity e) const;       // 世代對得上才活著(防懸空 handle)
};
```

創意點:**entity handle 帶世代**,delete 後 handle 立刻失效 — 配合你 `Pointers.hpp`
的語彙,把「資源所有權」變成引擎的核心語言。

### 3.3 Component storage — SoA 陣列,一個型別一張表

```cpp
// 內部:ComponentPool
// DynamicArray<T> 平鋪,entity 索引即陣列索引(SoA,快取友善)

struct Position { float x, y, z; };
struct Velocity { float x, y, z; };

world.AddComponent<Position>(entity, {0.f, 0.f, 0.f});
world.AddComponent<Velocity>(entity, {1.f, 0.f, 0.f});
```

### 3.4 View — 迭代器,系統的唯一天窗

```cpp
// 只讀取同時有 Position + Velocity 的 entity,拿到底層平鋪陣列
for (auto [entity, pos, vel] : world.View<Position, Velocity>())
{
    pos->x += vel->x * dt;
}
```

- View 直接 iterates 你 `DynamicArray` 的連續記憶體 → cache-friendly、零虛擬呼叫。
- Systems = 純函式/仿函式,吃 `World & + FrameClock` → **standalone 可測**
  (你已有 `SystemModuleStandalone.cpp` 的模式,直接沿用)。

### 3.5 確定性契約(Sim 層的憲法)

| 允許 | 禁止 |
|---|---|
| 固定步進(FixedUpdate,如 60Hz) | `std::chrono` 牆鐘 |
| 確定性 RNG(如 splitmix64,種子存 World) | `rand()` / `mt19937` 無種子 |
| 整數 / 定點數模擬 | 從 GPU 讀回資料 |
| 純數學(你的 Math) | 浮點依賴編譯器優化順序(必要時 fixed-point) |

**回報:內建 Replay。** 記錄輸入事件流 → 重播 → 一模一樣的結果。
除錯時「回到 3 秒前」、網路同步、比賽錄影,全部免費。

---

## 4. View Layer:Frame = 唯一的貨幣

### 4.1 Frame — 渲染命令流(Sim 產出,View 執行)

```cpp
// Display/Frame.hpp (新)
class Frame
{
  public:
    enum class Cmd
    {
        BeginPass, SetCamera, DrawMesh, DrawText,
        SetMaterial, EndPass, Present
    };

    void DrawMesh(const MeshRef &mesh, const MaterialRef &mat,
                  const Mat4 &transform);   // 內部 append Cmd
};
```

Sim/遊戲邏輯 **只會 append 命令**,不知道也不在乎執行者是誰。

### 4.2 IFrameExecutor — 後端的新介面(取代肥大 IRenderer)

```cpp
// Display/IFrameExecutor.hpp (新)
class IFrameExecutor
{
  public:
    virtual ~IFrameExecutor() = default;
    virtual bool Initialize(const Window *pWindow) = 0;
    virtual void Execute(const Frame &frame) = 0;   // 唯一的核心方法
    virtual void OnWindowResized(long w, long h) = 0;
};
```

| 現在(IRenderer,每後端各寫一套) | 之後(IFrameExecutor) |
|---|---|
| LoadScene / LoadGUILayout | 沒了 — Scene/GUI 只是 Frame 的來源之一 |
| AddVertexShader / AddPixelShader | 收進 Material/Asset 管理 |
| ApplyCamera / OnCameraChanged | `SetCamera` 命令 |
| Render(Scene) / Render(GUILayout) | `Execute(Frame)` 一體適用 |
| Update() | 併入模組 Update |

### 4.3 現有程式碼的命運(對照表)

| 現在 | 之後 |
|---|---|
| `IRenderer`(Vulkan/DX/GL 各一套) | `IFrameExecutor` + 三個薄 executor |
| `App/DirectX` + `App/Vulkan`(重複 main loop) | 刪掉,合併成**單一** `MainLoop`(固定步進 + 幀率上限 + 命令錄製) |
| `Scene`(retained 列表) | 變成 View 模組:訂閱 World 事件 → 產生 Frame;或保留為相容 adapter |
| `GUILayer/GUILayout/Button…` | 變成 GUI 模組,也輸出 Frame 命令(未來可轉 immediate mode) |
| `Shader/Texture` 載入 | 收進 AssetManager(非同步) |
| `Camera` | Sim 側的資料(ProjectionMatrix 由 View 算) |

---

## 5. Foundation 的擴充:記憶體與 Job

1. **Arena 分配器**(`System/Memory/`):
   - `FrameArena`:每幀用完整塊丟棄 → **一幀零 free**。
   - `AssetArena`:資產生命週期專用。
   - 你的容器全部可以吃外部分配器(Array/DynamicArray 加 allocator 參數)。
2. **Job 系統**(`System/Job/`):
   - 輕量 worker pool(`std::thread` + 你的 `DynamicArray` 當佇列)。
   - ECS 系統可以 parallel-for(View 分段)。
   - 資產非同步載入的第一步。
3. **AssetManager**(`System/Asset/`):
   - `HashTable<Str, AssetRef>` + 引用計數;內容定址(content hash)去重。
   - 非同步載入 → `AssetLoadedEvent` 廣播(型別事件,View 訂閱)。

---

## 6. Roadmap(對應你 issue-first 的工作流)

| 階段 | 內容 | 驗收(TestMain) |
|---|---|---|
| **P1 — Bus & Context** | 正式化模組介面 + EngineContext;`Event` 補 unsubscribe 安全(送 event 時 remove) | Event 多訂閱者/解除訂閱測試(已有,補邊界) |
| **P2 — Frame & Executor** | 定義 `Frame` 命令集;Vulkan 先改 `IFrameExecutor`;合併單一 MainLoop;刪 DX/VK App 重複碼 | 無 GPU CI 上 Frame 序列化測試(命令流可印出比對) |
| **P3 — ECS Core** | EntityRegistry + ComponentPool + View,全部建在你的容器上 | sparse set 世代/重生/刪除壓力測試;standalone |
| **P4 — Sim/View 分離** | World 進 Sim 層;固定步進 + 輸入錄製;View 模組把 World 投影成 Frame | **Replay 測試:錄 10 秒輸入 → 重播 → 狀態 hash 相等** |
| **P5 — Assets & Jobs** | AssetManager + worker pool + 非同步貼圖/Shader | 非同步載入完成事件測試 |
| **P6 — ChiaApp 重構** | ChiaApp 改成第一個 module 組裝的範例;GUI 走 Frame | ChiaApp 正常渲染(人工驗收) |

每一階段都是獨立 issue + commit(`Closes #N`),與你現有流程一致。

---

## 7. 風險與取捨

- **ECS 效能天花板**:View 全覽掃描對中小型場景綽綽有餘;若要百萬 entity,未來在
  ComponentPool 加 chunked 儲存,介面不變。**先求正確與可測,不求最速。**
- **確定性 vs 浮點**:物理用 float 在 x86 上通常夠確定(不跨平台需求時);
  跨平台對戰才需要 fixed-point。保留契約,延後實作。
- **Frame 序列化成本**:每幀建命令流有 allocation 成本 → FrameArena 解決(整塊丟棄)。
- **Event 的效能**:現行 `Event::Invoke` 走 HashTable 迭代,對每幀高頻事件(如 input)
  可接受;若瓶頸,加「per-frame direct dispatch」快徑,介面不變。

---

## 8. 一句話總結

> **ChiaEngine v2 = 確定性的 Sim 心臟 + Frame 貨幣 + 模組匯流排,
> 全部長在你自己的容器上。**
> 渲染只是投影,測試就是重播,而重播就是除錯。
