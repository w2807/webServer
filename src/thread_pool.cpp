#include "thread_pool.h"

#include <cstddef>
#include <functional>
#include <mutex>
#include <utility>

threadpool::ThreadPool::ThreadPool(size_t threads) {
    for (size_t i = 0; i < threads; i++) {
        workers_.emplace_back([this] {
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(this->queue_mutex_);
                    this->condition_.wait(lock, [this] {
                        return this->stop_ || !this->tasks_.empty();
                    });
                    if (this->stop_ && this->tasks_.empty()) {
                        return;
                    }
                    task = std::move(this->tasks_.front());
                    this->tasks_.pop();
                }
                task();
            }
        });
    }
}

threadpool::ThreadPool::~ThreadPool() {
    stop_ = true;
    condition_.notify_all();
    for (auto&& worker : workers_) {
        worker.join();
    }
}

void threadpool::ThreadPool::wait() {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    condition_.wait(lock, [this] { return this->tasks_.empty(); });
}
