#ifndef PONG_TYPES_HPP
#define PONG_TYPES_HPP

#include "Geometry/3D/Point3D.hpp"
#include <cstdint>

 // Pong 的 Sim 層 POD 元件(trivially copyable、無 padding 陷阱)。
 // position 以「世界單位」表示:場地 X ∈ [-9, 9]、Y ∈ [-6, 6] 的矩形。
 // 所有元件的資料欄位都是 float,ComponentPool::Add 的零初始化保證
 // World::Hash() 的 raw-byte FNV 位元級穩定 → 可被 SimRecorder 重播驗證。
namespace pong
{

// 球的物理狀態(每 tick 由 PongSystem::FixedUpdate 推進)。
// 與 RenderComponent 分離:純資料、無 View 依賴,Sim 層完全確定性。
struct BallComponent
{
    Point3D position;   // 球心(世界單位)
    Point3D velocity;   // 速度(世界單位/秒)
};

// 一顆球拍的核心資料:世界單位的中心位置 + 半高(half-height)。
// 碰撞判定由 PongSystem 直接算(world-space AABB sphere — 見
// Physics/Overlap.hpp 的 AABB-球 overlap,那正是為 P7a 做的)。
struct PaddleComponent
{
    Point3D position;   // 中心
    float halfHeight;   // 半高(3D;z 軸指不進球場,永遠 0)
    float speed;        // 移動速率(世界單位/秒;僅 CPU 拍使用,玩家由輸入驅動)
};

// 每顆拍一個 OptionalPaddlePlayer:玩家拍(Player=One)由輸入移動,
// CPU 拍(Mode=Cpu)由追球 AI 移動。
struct OptionalPaddlePlayer
{
    enum class Player : uint8_t
    {
        One = 0, // 左拍:W/S 或 Up/Down
        Two = 1, // 右拍:Up/Down(玩家模式)
    } player;

    bool isCpu; // true = CPU 控制(替代輸入源)
};

// Render 標記:有這元件的 entity 是「拍子」,View 畫成矩形。
// (Sim 不知道 View 怎麼畫;這只是 View 查詢的型別標記。)
struct PaddleEntityTag
{
};

// 選單 / 記分板上方文字的純資料標記(由 HUD 模組產生 Frame 命令)。
struct HudTextTag
{
};

} // namespace pong

#endif // PONG_TYPES_HPP