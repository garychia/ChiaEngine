#ifndef OWNERSHIP_TEST_HPP
#define OWNERSHIP_TEST_HPP

#include "Test.hpp"
#include "Data/Pointers.hpp"
#include <atomic>
#include <thread>
#include <type_traits>
#include <vector>

// #86:專屬的所有權測試。
//   * Ptr 複製語意已刪除(編譯期驗證)→ 不再有「隱式 copy ctor 淺拷貝 → 雙 delete」。
//   * SharedPtr 跨執行緒複製 → atomic 計數;物件恰好在最後一份釋放時銷毀一次
//     (Tracked.liveCount 歸零 = 無雙 delete、無洩漏)。
//   * WeakPtr 不再暴露 operator*/->/bool:唯一解引用通道 Lock()。
//     「最後釋放 → 銷毀」與「Lock 升級」互斥 → 交錯也不懸空。
struct Tracked
{
    static inline std::atomic<int> liveCount; // header-only test → inline 定義避免多 TU 重定義
    Tracked()
    {
        liveCount.fetch_add(1, std::memory_order_relaxed);
    }
    ~Tracked()
    {
        liveCount.fetch_sub(1, std::memory_order_relaxed);
    }
};

class OwnershipTest : public Test
{
  public:
    OwnershipTest(const std::string &name = "") : Test(name) {}

    bool Run() override
    {
        TEST_MESSAGE("Ptr Copy Deleted");
        static_assert(!std::is_copy_constructible<Ptr<int>>::value,
                      "Ptr must not be copy-constructible (#86 double-delete hazard).");
        static_assert(!std::is_copy_assignable<Ptr<int>>::value,
                      "Ptr must not be copy-assignable (#86 implicit shallow copy).");
        EXPECT_TRUE(true, "Ptr copy ctor/assign are deleted at compile time.", true);
        SUCCESS_MESSAGE("Ptr Copy Deleted");

        TEST_MESSAGE("WeakPtr Lock After Free");
        SharedPtr<Tracked> strong = SharedPtr<Tracked>::Construct();
        WeakPtr<Tracked> weak(strong);
        EXPECT_TRUE(weak.IsValid(), "WeakPtr valid while a strong reference exists.", true);
        EXPECT_TRUE(weak.Lock().IsValid(), "Lock succeeds while target alive.", true);
        strong.Release();
        EXPECT_TRUE(!weak.IsValid(), "WeakPtr invalid after the last strong release.", true);
        EXPECT_TRUE(!weak.Lock().IsValid(), "Lock on a dead target returns an empty SharedPtr.", true);
        weak.Clear();
        SUCCESS_MESSAGE("WeakPtr Lock After Free");

        TEST_MESSAGE("SharedPtr Cross-Thread Copy");
        {
            SharedPtr<Tracked> shared = SharedPtr<Tracked>::Construct();
            EXPECT_TRUE(Tracked::liveCount.load(std::memory_order_relaxed) == 1, "Object alive exactly once.", true);
            std::vector<std::thread> threads;
            for (int t = 0; t < 8; t++)
            {
                // 傳值 → 每個執行緒一份 strong;密集複製/釋放逼 atomic 計數交錯
                threads.emplace_back([shared]() {
                    SharedPtr<Tracked> l1(shared), l2(shared);
                    for (int i = 0; i < 20000; i++)
                    {
                        SharedPtr<Tracked> tmp(l1);
                        l1 = tmp;
                        l1 = l2;
                    }
                });
            }
            for (auto &thread : threads)
                thread.join();
            EXPECT_TRUE(Tracked::liveCount.load(std::memory_order_relaxed) == 1, "Object alive once while shared holds.",
                        true);
        }
        EXPECT_TRUE(Tracked::liveCount.load(std::memory_order_relaxed) == 0,
                    "Exactly one destroy: no double-delete, no leak.", true);
        SUCCESS_MESSAGE("SharedPtr Cross-Thread Copy");

        TEST_MESSAGE("WeakPtr Lock Release Race");
        for (int iter = 0; iter < 16; iter++)
        {
            SharedPtr<Tracked> source = SharedPtr<Tracked>::Construct();
            WeakPtr<Tracked> weak(source);
            std::atomic<bool> start(false);
            std::vector<std::thread> threads;
            for (int t = 0; t < 8; t++)
            {
                threads.emplace_back([weak, &start]() {
                    while (!start.load(std::memory_order_acquire))
                        std::this_thread::yield();
                    for (int i = 0; i < 5000; i++)
                        // 存亡皆安全:活 → 升級;已亡 → 空。兩者皆經 TryLock 互斥路徑。
                        weak.Lock();
                });
            }
            start.store(true, std::memory_order_release);
            source.Release(); // 與 8 個執行緒的 Lock 交錯 → 鍛煉 Lock-vs-destroy 互斥
            for (auto &thread : threads)
                thread.join();
        }
        EXPECT_TRUE(Tracked::liveCount.load(std::memory_order_relaxed) == 0,
                    "Lock/release race left no leaked or double-destroyed object.", true);
        SUCCESS_MESSAGE("WeakPtr Lock Release Race");

        return true;
    }
};

#endif