#include "thread_pool.h"

#include <cassert>
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
    wait_for_completion();
    {
        std::scoped_lock lock(mutex_);
        stopping_ = true;
    }
    cv_.notify_all();
    workers_.clear(); // jthreads join on destruction
}

void ThreadPool::enqueue(std::function<void()> task) {
    {
        std::scoped_lock lock(mutex_);
        if (stopping_) {
            return;
        }
        tasks_.push(std::move(task));
    }
    cv_.notify_one();
}

void ThreadPool::wait_for_completion() {
    std::unique_lock lock(mutex_);
    idle_cv_.wait(lock, [this]() { return tasks_.empty() && active_tasks_ == 0; });
}

void ThreadPool::worker(std::stop_token token) {
    while (!token.stop_requested()) {
        std::function<void()> task;
        {
            std::unique_lock lock(mutex_);
            cv_.wait(lock, token, [this] { return stopping_ || !tasks_.empty(); });
            if (stopping_ && tasks_.empty()) {
                break;
            }
            assert(!tasks_.empty());
            task = std::move(tasks_.front());
            tasks_.pop();
            ++active_tasks_;
        }

        if (task) {
            task();
        }

        {
            std::scoped_lock lock(mutex_);
            if (active_tasks_ > 0) {
                --active_tasks_;
            }
            if (tasks_.empty() && active_tasks_ == 0) {
                idle_cv_.notify_all();
                if (stopping_) {
                    cv_.notify_all();
                }
            }
        }
    }
    // ensure idle waiters can unblock if we exit while tasks are done
    std::scoped_lock lock(mutex_);
    if (tasks_.empty() && active_tasks_ == 0) {
        idle_cv_.notify_all();
    }
}

} // namespace bayeselo
