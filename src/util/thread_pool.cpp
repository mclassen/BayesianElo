#include "thread_pool.h"

#include <algorithm>

namespace bayeselo {

ThreadPool::ThreadPool(std::size_t threads) : thread_count_(threads == 0 ? 1 : threads), latch_(thread_count_) {
    for (std::size_t i = 0; i < thread_count_; ++i) {
        threads_.emplace_back([this](std::stop_token token) { worker(token); });
    }
}

ThreadPool::~ThreadPool() {
    wait();
}

void ThreadPool::wait() {
    shutdown_.store(true);
    {
        std::lock_guard lock(mutex_);
        cv_.notify_all();
    }
    latch_.wait();
}

void ThreadPool::worker(std::stop_token token) {
    while (!token.stop_requested()) {
        std::function<void()> task;
        {
            std::unique_lock lock(mutex_);
            cv_.wait(lock, token, [&]() { return shutdown_.load() || !tasks_.empty(); });
            if (tasks_.empty()) {
                break;
            }
            task = std::move(tasks_.front());
            tasks_.pop_front();
        }
        if (task) task();
    }
    latch_.count_down();
}

}  // namespace bayeselo
