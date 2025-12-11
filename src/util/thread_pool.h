#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <stop_token>
#include <thread>
#include <vector>

namespace bayeselo {

class ThreadPool {
public:
    explicit ThreadPool(std::size_t threads);
    ~ThreadPool();

    void enqueue(std::function<void()> task);
    void wait_for_completion();
    void shutdown();

private:
    void worker(std::stop_token token);

    std::mutex mutex_;
    std::condition_variable_any cv_;
    std::condition_variable_any idle_cv_;
    std::queue<std::function<void()>> tasks_;
    std::vector<std::jthread> workers_;
    bool stopping_{false};
    std::size_t active_tasks_{0};
};

} // namespace bayeselo
