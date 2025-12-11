#include "thread_pool.h"

#include <utility>

namespace bayeselo {

ThreadPool::ThreadPool(std::size_t threads) {
    if (threads == 0) {
        threads = std::max<std::size_t>(1, std::thread::hardware_concurrency());
    }
    for (std::size_t i = 0; i < threads; ++i) {
        workers_.emplace_back([this](std::stop_token token) { worker(token); });
    }
}

ThreadPool::~ThreadPool() { shutdown(); }

void ThreadPool::shutdown() {
    {
        std::scoped_lock lock(mutex_);
        stopping_ = true;
    }
    cv_.notify_all();
}

void ThreadPool::enqueue(std::function<void()> task) {
    {
        std::scoped_lock lock(mutex_);
        tasks_.push(std::move(task));
    }
    cv_.notify_one();
}

void ThreadPool::worker(std::stop_token token) {
    while (!token.stop_requested()) {
        std::function<void()> task;
        {
            std::unique_lock lock(mutex_);
            cv_.wait(lock, token, [this] { return stopping_ || !tasks_.empty(); });
            if (stopping_ && tasks_.empty()) {
                return;
            }
            task = std::move(tasks_.front());
            tasks_.pop();
        }
        if (task) {
            task();
        }
    }
}

} // namespace bayeselo

