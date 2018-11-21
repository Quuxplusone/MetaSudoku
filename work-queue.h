#pragma once

#include <atomic>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

struct ProducerShutDownException {};
struct ConsumerShutDownException {};

template<class T>
class ConcurrentQueue {
    std::queue<T> q_;
    mutable std::mutex mtx_;
    bool shutdown_when_empty_ = false;
    bool shutdown_ = false;
    std::condition_variable cv_;
    bool consumer_has_been_notified_ = false;
    std::condition_variable wait_cv_;

public:
    size_t size() const {
        std::unique_lock<std::mutex> lk(mtx_);
        return q_.size();
    }
    void push(T&& t) {
        std::unique_lock<std::mutex> lk(mtx_);
        assert(!shutdown_when_empty_ && "shouldn't still be pushing in this case");
        if (shutdown_) {
            throw ProducerShutDownException();
        }
        q_.push(std::move(t));
        lk.unlock();
        cv_.notify_one();
    }
    void push(const T& t) {
        std::unique_lock<std::mutex> lk(mtx_);
        assert(!shutdown_when_empty_ && "shouldn't still be pushing in this case");
        if (shutdown_) {
            throw ProducerShutDownException();
        }
        q_.push(t);
        lk.unlock();
        cv_.notify_one();
    }
    T pop() {
        std::unique_lock<std::mutex> lk(mtx_);
        while (q_.empty()) {
            if (shutdown_ || shutdown_when_empty_) {
                consumer_has_been_notified_ = true;
                wait_cv_.notify_all();
                throw ConsumerShutDownException();
            }
            cv_.wait(lk);
        }
        if (shutdown_) {
            consumer_has_been_notified_ = true;
            wait_cv_.notify_all();
            throw ConsumerShutDownException();
        }
        T result = std::move(q_.front());
        q_.pop();
        lk.unlock();
        return result;
    }
    bool try_pop(T& value) {
        std::unique_lock<std::mutex> lk(mtx_);
        if (q_.empty()) {
            return false;
        }
        value = std::move(q_.front());
        q_.pop();
        return true;
    }
    void shutdown_from_producer_side() {
        std::unique_lock<std::mutex> lk(mtx_);
        shutdown_ = true;
        lk.unlock();
        cv_.notify_all();
    }
    void shutdown_from_consumer_side() {
        std::unique_lock<std::mutex> lk(mtx_);
        shutdown_ = true;
        consumer_has_been_notified_ = true;
        lk.unlock();
        cv_.notify_all();
        wait_cv_.notify_all();
    }
    void shutdown_when_empty() {
        std::unique_lock<std::mutex> lk(mtx_);
        shutdown_when_empty_ = true;
        lk.unlock();
        cv_.notify_all();
    }
    void wait() {
        std::unique_lock<std::mutex> lk(mtx_);
        assert(shutdown_ || shutdown_when_empty_);
        while (!consumer_has_been_notified_) {
            wait_cv_.wait(lk);
        }
    }
};

template<class State, class Task, int NumThreads, class CRTP>
class RoundRobinPool {
    std::thread workers_[NumThreads];
    ConcurrentQueue<Task> queues_[NumThreads];
    State states_[NumThreads];
    int robin_ = 0;

    CRTP& as_crtp() { return static_cast<CRTP&>(*this); }

public:
    template<class F>
    void for_each_state(const F& f) {
        for (int i=0; i < NumThreads; ++i) {
            f(states_[i]);
        }
    }

    std::string queue_sizes() const {
        char buffer[NumThreads * 100];
        char *p = buffer;
        for (int i=0; i < NumThreads; ++i) {
            size_t n = queues_[i].size();
            p += snprintf(p, 100, "%zu ", n);
        }
        return std::string(buffer, p - 1);
    }

    void rebalance_queues() {
        // No pushing should be happening during this operation.
        // Popping can still be happening, though.
        int biggest_queue_index = 0;
        size_t biggest_queue_size = 0;
        for (int i=0; i < NumThreads; ++i) {
            size_t n = queues_[i].size();
            if (n > biggest_queue_size) {
                biggest_queue_index = i;
                biggest_queue_size = n;
            }
        }
        auto& q = queues_[biggest_queue_index];
        Task task;
        for (int i=0; i < biggest_queue_size; ++i) {
            if (!q.try_pop(task)) {
                break;
            }
            this->push(std::move(task));
        }
    }

    void push(Task&& task) {
        robin_ += 1;
        robin_ %= NumThreads;
        queues_[robin_].push(std::move(task));
    }
    void push(const Task& task) {
        robin_ += 1;
        robin_ %= NumThreads;
        queues_[robin_].push(task);
    }
    void start_threads() {
        for (int i=0; i < NumThreads; ++i) {
            workers_[i] = std::thread([this, i]() {
                while (true) {
                    try {
                        auto task = queues_[i].pop();
                        as_crtp().process(states_[i], std::move(task));
                    } catch (const ConsumerShutDownException&) {
                        queues_[i].shutdown_from_consumer_side();
                        return;
                    } catch (...) {
                        assert(false);
                    }
                }
            });
        }
    }
    void shutdown_from_producer_side() {
        for (int i=0; i < NumThreads; ++i) {
            queues_[i].shutdown_from_producer_side();
        }
    }
    void shutdown_when_empty() {
        for (int i=0; i < NumThreads; ++i) {
            queues_[i].shutdown_when_empty();
        }
    }
    void wait() {
        for (int i=0; i < NumThreads; ++i) {
            queues_[i].wait();
        }
    }
    ~RoundRobinPool() {
        if (workers_[0].joinable()) {
            for (int i=0; i < NumThreads; ++i) {
                queues_[i].shutdown_from_producer_side();
                queues_[i].shutdown_from_consumer_side();
            }
            for (int i=0; i < NumThreads; ++i) {
                workers_[i].join();
            }
        }
    }
};
