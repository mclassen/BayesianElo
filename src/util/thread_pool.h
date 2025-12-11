#pragma once

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <functional>
#include <latch>
#include <mutex>
#include <stop_token>
#include <thread>
#include <vector>
#include <utility>
#include <atomic>

namespace bayeselo {

class ThreadPool {
public:
    explicit ThreadPool(std::size_t threads);
    ~ThreadPool();

    template <typename Fn>
    void enqueue(Fn&& fn) const {
        {
            std::lock_guard lock(mutex_);
            tasks_.emplace_back(std::forward<Fn>(fn));
        }
        cv_.notify_one();
    }

    void wait();

private:
    void worker(std::stop_token token);

    mutable std::mutex mutex_{};
    mutable std::condition_variable_any cv_{};
    mutable std::deque<std::function<void()>> tasks_{};
    std::vector<std::jthread> threads_{};
    std::latch latch_;
    std::size_t thread_count_{};
    std::atomic<bool> shutdown_{false};
};

}  // namespace bayeselo
