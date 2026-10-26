#ifndef PADDING_HPP
#define PADDING_HPP

#include <cstddef>
#include <type_traits>

// 編譯期「無隱含 padding」guard —— World::Hash() 逐位元組雜湊的前提。
//
// 為什麼需要:Hash() 讀 sizeof(T) 個位元組,而 ABI 決定的隱含 padding byte 值
// 是 unspecified —— 邏輯相同的兩個元件可能產生不同 hash。實測(gcc 15.2 / clang 18.1):
// ColliderComponent{enum class Kind : uint8_t; float data[7];} 的 sizeof 是 32,成員
// 只佔 29 byte;clang -O2 會把來源物件的垃圾 padding 一起複製進池子(raw bytes
// "00 00 80 3f 07 aa aa aa"),gcc -O2 恰好不會清掉 → 同值的兩個 world hash 不同,
// 而且同一份程式在兩條 compiler leg 上結果也不同。見 #129 / #142。
//
// 原理:sizeof(T) >= Σ sizeof(members) + padding,所以等號成立 ⟺ 沒有任何 padding
// byte(含尾端對齊補齊)。不需要 offsetof,不需要反射,對 float 成員同樣有效。
//
// 注意:
// - 必須列出「所有」non-static data member。漏列會讓總和變小,assert 當場失敗,
//   所以這份清單不會靜默過期。
// - 有 base class 或 vtable 的型別無法通過(它們的位元組不在成員清單裡)—— 元件
//   請用平坦的 struct,不要繼承。
// - 空型別(tag 元件)沒有成員也沒有 padding,視為通過;它那 1 byte 的儲存值
//   unspecified,所以 ComponentPool::Hash() 對空型別刻意不讀那顆 byte(見該處)。
// - 這裡只檢查 padding,不要求 trivially copyable:Point3D 因自訂 operator= 而不是
//   trivially copyable,但它的物件表現仍是定義良好的 12 byte。
//
// 為什麼不用 std::has_unique_object_representations:實測 gcc 15.2 與 clang 18.1 對
// 「任何含 float 的 struct」都回 false —— 包含完全無 padding 的 struct{float,float,float}
// (±0.0 與 NaN payload 讓標準意義下的「同值」不保證同位元組),所以那條 trait 不能
// 拿來當這個 guard。
template <class T, class... Member> constexpr bool NoPadding(Member T::*...pMember) noexcept
{
    if constexpr (std::is_empty_v<T>)
        return true; // 沒有成員 → 沒有隱含 padding
    else
        return sizeof(T) == (sizeof(((T *)nullptr)->*pMember) + ... + 0);
}

// 用法(緊接在型別定義之後):
//   CHIA_PADDING_FREE(Position, &Position::x, &Position::y, &Position::z);
#define CHIA_PADDING_FREE(TYPE, ...)                                                                              \
    static_assert(::NoPadding<TYPE>(__VA_ARGS__),                                                                 \
                  #TYPE " has implicit padding: World::Hash() reads sizeof(T) bytes, so a padded type hashes "   \
                        "differently per compiler (#129 / #142). Declare explicit filler members that are "       \
                        "always initialized, or specialize HashComponent<T>.")

#endif // PADDING_HPP
