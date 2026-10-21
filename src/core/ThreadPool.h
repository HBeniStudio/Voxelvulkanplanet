#pragma once

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace voxel::core {

class ThreadPool {
public:
    explicit ThreadPool(size_t threadCount = 0);
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    void enqueue(std::function<void()> task);

    /// Run one queued task on the calling thread if any are waiting. Returns whether a task ran.
    bool tryExecuteOne();

    /// Block until the queue is empty and no task is executing.
    bool waitIdle(std::chrono::milliseconds maxWait = std::chrono::milliseconds::max());

private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::condition_variable idleCv_;
    size_t inFlight_ = 0;
    bool stop_ = false;

    void workerLoop();
    void runTask(std::function<void()> task);
};

} // namespace voxel::core
