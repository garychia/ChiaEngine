#ifndef TYPE_NAME_HPP
#define TYPE_NAME_HPP

#include <cstddef>
#include <string_view>

// 型別的人看得懂名字(回傳的 string_view 指向靜態字串,生命期同程式)。
//
// 用途:稽核 —— replay 記錄裡「註冊了哪些元件型別、依什麼順序」(#150)。
//
// 它不是 hash 輸入,也不該變成 hash 輸入:各家 compiler 的名字格式不同
// (GCC "[with T = X]"、Clang "[T = X]"、MSVC "TypeNameRaw<struct X>(void)"),
// 拿它算 hash 等於把 compiler 依賴重新塞回 World::Hash() —— 那正是 #147 拿掉的東西。
template <class T> constexpr const char *TypeNameRaw() noexcept
{
    return __PRETTY_FUNCTION__;
}

constexpr bool StartsWith(std::string_view text, std::string_view prefix) noexcept
{
    return text.size() >= prefix.size() && text.substr(0, prefix.size()) == prefix;
}

// 從 __PRETTY_FUNCTION__ 取出型別名,去掉編譯器自己的前後綴。認得的格式:
//   GCC   : "const char* TypeNameRaw() [with T = pong::BallComponent]"
//   Clang : "const char* TypeNameRaw() [T = pong::BallComponent]"
//   MSVC  : "const char* __cdecl TypeNameRaw<struct pong::BallComponent>(void)"
// 都認不出來時回傳整串 —— 寧可醜,也不要讓稽核欄位靜默變空。
constexpr std::string_view ExtractTypeName(const char *pText) noexcept
{
    if (pText == nullptr)
        return std::string_view();

    // 1) GCC / Clang:"T = " 之後到 ']' 或 ';' 之前。
    //    (p[1] 起都是短路求值,不會讀過字串結尾。)
    for (const char *p = pText; p[0] != '\0'; ++p)
    {
        if (p[0] == 'T' && p[1] == ' ' && p[2] == '=' && p[3] == ' ')
        {
            const char *pBegin = p + 4;
            const char *pEnd = pBegin;
            while (pEnd[0] != '\0' && pEnd[0] != ']' && pEnd[0] != ';')
                ++pEnd;
            return std::string_view(pBegin, static_cast<size_t>(pEnd - pBegin));
        }
    }

    // 2) MSVC:函式名後的第一個 '<',配對到深度歸零的 '>',再剝掉 struct/class/enum。
    for (const char *p = pText; p[0] != '\0'; ++p)
    {
        if (p[0] != '<')
            continue;

        const char *pBegin = p + 1;
        const char *pEnd = pBegin;
        int depth = 1;
        while (pEnd[0] != '\0' && depth > 0)
        {
            if (pEnd[0] == '<')
                ++depth;
            else if (pEnd[0] == '>')
                --depth;
            if (depth == 0)
                break;
            ++pEnd;
        }

        std::string_view name(pBegin, static_cast<size_t>(pEnd - pBegin));
        if (StartsWith(name, "struct "))
            name.remove_prefix(7);
        else if (StartsWith(name, "class "))
            name.remove_prefix(6);
        else if (StartsWith(name, "enum "))
            name.remove_prefix(5);
        return name;
    }

    return std::string_view(pText);
}

template <class T> constexpr std::string_view TypeName() noexcept
{
    return ExtractTypeName(TypeNameRaw<T>());
}

#endif // TYPE_NAME_HPP
