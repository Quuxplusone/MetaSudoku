#pragma once

#include <atomic>
#include <mutex>
#include <queue>
#include <thread>

struct ShutDownException {};

template<class T>
struct ConcurrentQueue {
    std::queue<T> q_;
    std::mutex mtx_;
    std::condition_variable cv_;
    bool shutdown_ = false;
    std::condition_variable shutdown_cv_;

    void push(T&& t) {
        std::unique_lock<std::mutex> lk(mtx_);
        if (shutdown_) {
            throw ShutDownException();
        }
        q_.push(std::move(t));
        lk.unlock();
        cv_.notify_one();
    }
    void push(const T& t) {
        std::unique_lock<std::mutex> lk(mtx_);
        if (shutdown_) {
            throw ShutDownException();
        }
        q_.push(t);
        lk.unlock();
        cv_.notify_one();
    }
    T pop() {
        std::unique_lock<std::mutex> lk(mtx_);
        while (q_.empty()) {
            cv_.wait(lk);
            if (shutdown_) {
                throw ShutDownException();
            }
        }
        T result = std::move(q_.front());
        q_.pop();
        lk.unlock();
        return result;
    }
    void shutdown() {
        std::unique_lock<std::mutex> lk(mtx_);
        lk.unlock();
        shutdown_ = true;
        cv_.notify_one();
        shutdown_cv_.notify_one();
    }
    void wait() {
        std::unique_lock<std::mutex> lk(mtx_);
        while (!shutdown_) {
            shutdown_cv_.wait(lk);
        }
    }
};

template<class State, class Task, int NumThreads, class CRTP>
struct RoundRobinPool {
    std::thread workers_[NumThreads];
    ConcurrentQueue<Task> queues_[NumThreads];
    State states_[NumThreads];
    int robin_ = 0;

    CRTP& as_crtp() { return static_cast<CRTP&>(*this); }

    template<class F>
    void for_each_state(const F& f) {
        for (int i=0; i < NumThreads; ++i) {
            f(states_[i]);
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
                        as_crtp().process(states_[i], queues_[i].pop());
                    } catch (const ShutDownException&) {
                        queues_[i].shutdown();
                        return;
                    }
                }
            });
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
                queues_[i].shutdown();
            }
            for (int i=0; i < NumThreads; ++i) {
                workers_[i].join();
            }
        }
    }
};
