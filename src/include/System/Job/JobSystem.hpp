#ifndef JOBSYSTEM_HPP
#define JOBSYSTEM_HPP

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

// JobSystem:輕量 worker pool(std::thread + 互斥佇列)。
// v2 設計(System/Job/) — ECS parallel-for 與資產非同步載入的底層第一步。
//
// 確定性契約:JobSystem 本身不保證執行順序(多 worker 會搶佇列)。
// 觀測確定性由呼叫端把「等一下」當成明確 checkpoint 達成:
//  enqueue 一堆任務 → 做別的事 → WaitForIdle() 回來後才讀取結果。
// 同一組輸入 + WaitForIdle 之後,結果與 worker 數量/排程無關。
class JobSystem
{
  public:
    explicit JobSystem(size_t numWorkers = 0)
    {
        if (numWorkers == 0)
            numWorkers = std::thread::hardware_concurrency();
        if (numWorkers == 0)
            numWorkers = 1;
        workers.reserve(numWorkers);
        for (size_t i = 0; i < numWorkers; i++)
            workers.emplace_back(&JobSystem::WorkerLoop, this);
    }

    ~JobSystem()
    {
        StopAndJoin();
    }

    JobSystem(const JobSystem &) = delete;
    JobSystem &operator=(const JobSystem &) = delete;

    // 把任務丟進佇列;由任一 worker 取出執行。
    void Enqueue(std::function<void()> &&task)
    {
        {
            std::lock_guard<std::mutex> lock(mutex);
            tasks.emplace_back(std::move(task));
            pendingCount++;
        }
        cv.notify_one();
    }

    template <class Fn> void Enqueue(Fn &&fn)
    {
        Enqueue(std::function<void()>(std::forward<Fn>(fn)));
    }

    // 阻塞直到所有已入隊任務完成(測試 CHECKPOINT;不要拿去當每幀 sync)。
    void WaitForIdle()
    {
        std::unique_lock<std::mutex> lock(mutex);
        idle.wait(lock, [this] { return pendingCount == 0; });
    }

    // 停止並 join 所有 worker。未消化完的佇列任務會被丟棄。
    void StopAndJoin()
    {
        {
            std::lock_guard<std::mutex> lock(mutex);
            if (stopping)
                return;
            stopping = true;
        }
        cv.notify_all();
        for (std::thread &worker : workers)
        {
            if (worker.joinable())
                worker.join();
        }
        workers.clear();
    }

  private:
    void WorkerLoop()
    {
        for (;;)
        {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(mutex);
                cv.wait(lock, [this] { return stopping || !tasks.empty(); });
                if (stopping && tasks.empty())
                    return;
                task = std::move(tasks.front());
                tasks.pop_front();
            }

            task();

            {
                std::lock_guard<std::mutex> lock(mutex);
                pendingCount--;
            }
            idle.notify_all();
        }
    }

    std::vector<std::thread> workers;
    std::deque<std::function<void()>> tasks;
    std::mutex mutex;
    std::condition_variable cv;
    std::condition_variable idle;
    bool stopping = false;
    size_t pendingCount = 0;
};

#endif // JOBSYSTEM_HPP