#ifndef PONG_TYPES_HPP
#define PONG_TYPES_HPP

#include "Data/Padding.hpp"
#include "Geometry/3D/Point3D.hpp"
#include <cstdint>

// Pong 的 Sim 層 POD 元件。position 以「世界單位」表示:場地 X ∈ [-9, 9]、Y ∈ [-6, 6]。
// 每個元件都必須沒有隱含 padding(下面的 CHIA_PADDING_FREE 在編譯期把關):
// World::Hash() 逐位元組讀 sizeof(T),而隱含 padding 的值 unspecified,會讓同值的
// 兩個 world 產生不同 hash(clang -O2 會把來源的垃圾 padding 帶進池子)。見 #129 / #142。
// (舊註解說「ComponentPool::Add 的零初始化保證位元級穩定」是錯的,那個 memset 是死碼。)
namespace pong
{

// 球的物理狀態(每 tick 由 PongSystem::FixedUpdate 推進)。
// 與 RenderComponent 分離:純資料、無 View 依賴,Sim 層完全確定性。
struct BallComponent
{
    Point3D position;   // 球心(世界單位)
    Point3D velocity;   // 速度(世界單位/秒)
};

CHIA_PADDING_FREE(BallComponent, &BallComponent::position, &BallComponent::velocity);

// 一顆球拍的核心資料:世界單位的中心位置 + 半高(half-height)。
// 碰撞判定由 PongSystem 直接算(world-space AABB sphere — 見
// Physics/Overlap.hpp 的 AABB-球 overlap,那正是為 P7a 做的)。
struct PaddleComponent
{
    Point3D position;   // 中心
    float halfHeight;   // 半高(3D;z 軸指不進球場,永遠 0)
    float speed;        // 移動速率(世界單位/秒;僅 CPU 拍使用,玩家由輸入驅動)
};

CHIA_PADDING_FREE(PaddleComponent, &PaddleComponent::position, &PaddleComponent::halfHeight,
                  &PaddleComponent::speed);

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

CHIA_PADDING_FREE(OptionalPaddlePlayer, &OptionalPaddlePlayer::player, &OptionalPaddlePlayer::isCpu);

// Render 標記:有這元件的 entity 是「拍子」,View 畫成矩形。
// (Sim 不知道 View 怎麼畫;這只是 View 查詢的型別標記。)
struct PaddleEntityTag
{
};

// 空型別沒有成員也沒有隱含 padding;它那 1 byte 的儲存值 unspecified,所以
// ComponentPool::Hash() 對空型別刻意不讀那顆 byte(見 ComponentPool.hpp)。
static_assert(::NoPadding<PaddleEntityTag>(), "PaddleEntityTag: empty type, no padding.");

// 選單 / 記分板上方文字的純資料標記(由 HUD 模組產生 Frame 命令)。
struct HudTextTag
{
};

static_assert(::NoPadding<HudTextTag>(), "HudTextTag: empty type, no padding.");

} // namespace pong

#endif // PONG_TYPES_HPP
