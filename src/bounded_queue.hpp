#pragma once

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>

template <typename T>
class BoundedQueue
{
    static_assert(
        std::is_nothrow_move_constructible_v<T>,
        "Queue elements must support non-throwing move construction");

public:
    explicit BoundedQueue(std::size_t capacity)
        : capacity_(capacity)
    {
        if (capacity == 0)
        {
            throw std::invalid_argument(
                "Queue capacity must be greater than zero");
        }
    }

    BoundedQueue(const BoundedQueue &) = delete;
    BoundedQueue &operator=(const BoundedQueue &) = delete;

    bool push(T value)
    {
        { // we acquire lock inside inner scope
            // lock mutex
            std::unique_lock<std::mutex> lock(mutex_);

            // if lambda is false, mutex lock get released
            // then thread gets placed in not_full queue.
            // waiting for the queue to not be full
            not_full_.wait(lock, [this]
                           { return (closed_ || items_.size() < capacity_); });

            if (closed_)
            {
                return false;
            }

            items_.push_back(std::move(value));
        } // lock object dies here => no need to unlock

        // notify any waiting threads that the queue is not empty
        not_empty_.notify_one();
        return true;
    }

    std::optional<T> pop()
    {
        std::unique_lock<std::mutex> lock(mutex_);

        // if lamda is false, mutex lock get released and
        // wait for not_empty condition var to get notified
        not_empty_.wait(lock, [this]
                        { return closed_ || !items_.empty(); });

        if (items_.empty())
        {
            return std::nullopt;
        }

        // may return type T or may be empty
        std::optional<T> value(
            std::move(items_.front()));

        items_.pop_front();

        lock.unlock();
        not_full_.notify_one();

        return value;
    }

    void close()
    {
        {
            // cant use lock_guard with condition variables
            std::lock_guard<std::mutex> lock(mutex_);
            closed_ = true;
        } // lock destroyed

        // wake all sleeping threads
        not_empty_.notify_all();
        not_full_.notify_all();
    }

private:
    const std::size_t capacity_;

    std::deque<T> items_;
    bool closed_ = false;

    std::mutex mutex_;
    std::condition_variable not_empty_;
    std::condition_variable not_full_;
};