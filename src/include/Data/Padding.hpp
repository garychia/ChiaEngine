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
// 全域 fallback:沒有被 CHIA_PADDING_FREE 標記的型別一律視為「未證明」。
constexpr bool ChiaPaddingFreeMarker(...) noexcept
{
    return false;
}

namespace chia_padding_detail
{
// 成員型別檢查:算術 / enum / 空型別放行(陣列先去掉維度),其餘交給 ADL 標記。
template <class T> constexpr bool MemberIsChecked() noexcept
{
    using U = std::remove_all_extents_t<T>;
    if constexpr (std::is_arithmetic_v<U> || std::is_enum_v<U> || std::is_empty_v<U>)
        return true;
    else
        return ChiaPaddingFreeMarker(static_cast<U *>(nullptr));
}

template <class T, class... Member> constexpr bool MembersAreChecked(Member T::*...pMember) noexcept
{
    return (MemberIsChecked<Member>() && ...);
}
} // namespace chia_padding_detail

template <class T, class... Member> constexpr bool NoPadding(Member T::*...pMember) noexcept
{
    if constexpr (std::is_empty_v<T>)
        return true; // 沒有成員 → 沒有隱含 padding
    else
        return sizeof(T) == (sizeof(((T *)nullptr)->*pMember) + ... + 0) &&
               chia_padding_detail::MembersAreChecked<T>(pMember...);
}

// 用法(緊接在型別定義之後):
//   CHIA_PADDING_FREE(Position, &Position::x, &Position::y, &Position::z);
// 巢狀型別要各別標記(#148):sizeof(成員) 會把成員型別「自己」的 padding 一起算進去,
// 所以只比尺寸會讓「內部有 padding 的子結構」靜默通過 —— 實測 struct Inner{float;uint8_t;}
// 自己的檢查回 false,但 struct Outer{Inner in;} 回 true。標記 = 在型別所屬 namespace
// 發出 ChiaPaddingFreeMarker(TYPE *) 多載,靠 ADL 找到;沒有標記就編譯失敗。
#define CHIA_PADDING_FREE(TYPE, ...)                                                                              \
    static_assert(::NoPadding<TYPE>(__VA_ARGS__),                                                                 \
                  #TYPE " is not proven padding-free: World::Hash() reads sizeof(T) bytes, so implicit padding " \
                        "- or an unmarked nested member type - makes the hash compiler-dependent (#129 / "       \
                        "#142 / #148). List every leaf member, mark nested types with CHIA_PADDING_FREE as "    \
                        "well, or specialize HashComponent<T>.");                                             \
    constexpr bool ChiaPaddingFreeMarker(TYPE * /*pUnused*/) noexcept                                             \
    {                                                                                                             \
        return true;                                                                                              \
    }

#endif // PADDING_HPP
