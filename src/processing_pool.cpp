#include "processing_pool.hpp"

#include "payload.hpp"
#include "statistics.hpp"

#include <stdexcept>
#include <utility>

ProcessingPool::ProcessingPool(std::size_t thread_count, std::size_t queue_capacity)
    : tasks_(queue_capacity),
      results_(queue_capacity),
      remaining_threads_(thread_count)
{
    if (thread_count == 0)
    {
        throw std::invalid_argument(
            "Processing pool requires at least one thread");
    }

    threads_.reserve(thread_count);

    try
    {
        for (std::size_t index = 0; index < thread_count; ++index)
        {
            // construct a new thread in threads vector and start it immediately
            threads_.emplace_back([this]
                                  { run(); });
        }
    }
    catch (...)
    {
        // Wake any threads already started before construction failed.
        tasks_.close();
        results_.close();
        throw;
    }
}

ProcessingPool::~ProcessingPool()
{
    // Also permits destruction when the caller stops collecting results.
    tasks_.close();
    results_.close();

    // Destroying jthreads joins them while the other members still exist.
    threads_.clear();
}

bool ProcessingPool::submit(protocol::Message task)
{
    // check and throw errors present in worker threads
    rethrow_failure();

    const bool accepted = tasks_.push(std::move(task));

    // check again
    rethrow_failure();
    return accepted;
}

std::optional<protocol::Message> ProcessingPool::receive()
{
    auto result = results_.pop();

    rethrow_failure();
    return result;
}

void ProcessingPool::finish()
{
    tasks_.close();
}

// used by workers to record any errors
void ProcessingPool::record_failure(std::exception_ptr error)
{
    {
        std::lock_guard<std::mutex> lock(failure_mutex_);

        if (!failure_)
        {
            failure_ = error;
        }
    }

    tasks_.close();
    results_.close();
}

// check for erros within worker threads and thrown one if present
void ProcessingPool::rethrow_failure()
{
    std::exception_ptr error;

    {
        std::lock_guard<std::mutex> lock(failure_mutex_);
        error = failure_;
    }

    if (error)
    {
        std::rethrow_exception(error);
    }
}

void ProcessingPool::run()
{
    try
    {
        while (auto task = tasks_.pop())
        {
            if (task->type != protocol::MessageType::task)
            {
                throw std::invalid_argument(
                    "Processing pool expected a task message");
            }

            const auto samples = protocol::decode_task(task->payload);

            const auto statistics = calculate_statistics(samples);

            protocol::Message result{
                protocol::MessageType::result,
                task->task_id,
                protocol::encode_result(statistics)};

            if (!results_.push(std::move(result)))
            {
                break;
            }
        }
    }
    catch (...)
    {
        record_failure(std::current_exception());
    }

    if (remaining_threads_.fetch_sub(1) == 1)
    {
        results_.close();
    }
}