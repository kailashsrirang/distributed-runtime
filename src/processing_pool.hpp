#pragma once

#include "bounded_queue.hpp"
#include "message.hpp"

#include <atomic>
#include <cstddef>
#include <exception>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

class ProcessingPool
{
public:
    ProcessingPool(
        std::size_t thread_count,
        std::size_t queue_capacity);

    ~ProcessingPool();

    ProcessingPool(const ProcessingPool &) = delete;
    ProcessingPool &operator=(const ProcessingPool &) = delete;

    bool submit(protocol::Message task);

    std::optional<protocol::Message> receive();

    // Stop accepting tasks; finish tasks already accepted.
    void finish();

private:
    void run();
    void record_failure(std::exception_ptr error);
    void rethrow_failure();

    BoundedQueue<protocol::Message> tasks_;
    BoundedQueue<protocol::Message> results_;

    std::atomic<std::size_t> remaining_threads_;

    // thread to store any exception thrown by a worker thread, so that it can be rethrown in the main thread.
    std::mutex failure_mutex_;
    // exception ptr stored here
    std::exception_ptr failure_;

    // Declared last so threads are destroyed/joined before other members.
    std::vector<std::jthread> threads_;
};