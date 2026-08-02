#ifndef ENGINECONTEXT_HPP
#define ENGINECONTEXT_HPP

#include "Data/HashTable.hpp"
#include "TypeId.hpp"

// 服務註冊表:模組之間不直接耦合,透過 EngineContext 註冊 / 解析服務。
// - key 是編譯期型別 ID(見 TypeId.hpp,與 World 共用計數器)
// - 儲存用你自己的 HashTable<uint32_t, void *>
// - 解析不到回傳 nullptr,呼叫端自行決定要不要
class EngineContext
{
  private:
    HashTable<uint32_t, void *> services;

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
