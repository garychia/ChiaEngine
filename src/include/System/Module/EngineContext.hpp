#ifndef ENGINECONTEXT_HPP
#define ENGINECONTEXT_HPP

#include "Data/HashTable.hpp"

#include <cstdint>

// 服務註冊表:模組之間不直接耦合,透過 EngineContext 註冊 / 解析服務。
// - key 是編譯期型別 ID:每個 T 拿到一個唯一的 u32(函式內 static,無 RTTI)
// - 儲存用你自己的 HashTable<uint32_t, void *>
// - 解析不到回傳 nullptr,呼叫端自行決定要不要
class EngineContext
{
  private:
    HashTable<uint32_t, void *> services;

    static uint32_t NextTypeId()
    {
        static uint32_t next = 1;
        return next++;
    }

    template <class T> static uint32_t TypeId()
    {
        static const uint32_t id = NextTypeId();
        return id;
    }

  public:
    EngineContext() : services()
    {
    }

    template <class T> void RegisterService(T *pService)
    {
        services.Insert(TypeId<T>(), static_cast<void *>(pService));
    }

    template <class T> void UnregisterService()
    {
        services.Remove(TypeId<T>());
    }

    template <class T> T *ResolveService() const
    {
        HashTable<uint32_t, void *>::Iterator itr = services.Find(TypeId<T>());
        if (itr == services.Last())
            return nullptr;
        return static_cast<T *>(itr->Value());
    }
};

#endif // ENGINECONTEXT_HPP
