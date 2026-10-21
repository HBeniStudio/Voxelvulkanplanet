#include "core/ThreadPool.h"

#include <algorithm>

namespace voxel::core {

ThreadPool::ThreadPool(size_t threadCount) {
    if (threadCount == 0) {
        threadCount = std::max<size_t>(2, std::thread::hardware_concurrency());
    } else {
        threadCount = std::max<size_t>(2, threadCount);
    }
    workers_.reserve(threadCount);
    for (size_t i = 0; i < threadCount; ++i) {
        workers_.emplace_back([this] { workerLoop(); });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::lock_guard lock(mutex_);
        stop_ = true;
    }
    cv_.notify_all();
    for (std::thread& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void ThreadPool::enqueue(std::function<void()> task) {
    {
        std::lock_guard lock(mutex_);
        tasks_.push(std::move(task));
    }
    cv_.notify_one();
}

void ThreadPool::runTask(std::function<void()> task) {
    task();
    std::lock_guard lock(mutex_);
    --inFlight_;
    if (tasks_.empty() && inFlight_ == 0) {
        idleCv_.notify_all();
    }
}

bool ThreadPool::tryExecuteOne() {
    std::function<void()> task;
    {
        std::lock_guard lock(mutex_);
        if (tasks_.empty()) {
            return false;
        }
        task = std::move(tasks_.front());
        tasks_.pop();
        ++inFlight_;
    }
    runTask(std::move(task));
    return true;
}

bool ThreadPool::waitIdle(std::chrono::milliseconds maxWait) {
    std::unique_lock lock(mutex_);
    return idleCv_.wait_for(lock, maxWait, [this] { return tasks_.empty() && inFlight_ == 0; });
}

void ThreadPool::workerLoop() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock lock(mutex_);
            cv_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
            if (stop_ && tasks_.empty()) {
                return;
            }
            task = std::move(tasks_.front());
            tasks_.pop();
            ++inFlight_;
        }
        runTask(std::move(task));
    }
}

} // namespace voxel::core
