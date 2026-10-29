#include "dataset.hpp"
#include "message.hpp"
#include "payload.hpp"
#include "statistics.hpp"
#include "tcp.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <optional>
#include <stdexcept>
#include <vector>

namespace
{
    bool nearly_equal(double actual, double expected)
    {
        constexpr double absolute_tolerance = 1e-12;
        constexpr double relative_tolerance = 1e-12;

        const double scale =
            std::max(std::abs(actual), std::abs(expected));

        return std::abs(actual - expected) <= absolute_tolerance + relative_tolerance * scale;
    }

    void validate_result(
        const Statistics &actual,
        const Statistics &expected)
    {
        if (actual.count != expected.count || actual.minimum != expected.minimum || actual.maximum != expected.maximum || !nearly_equal(actual.mean, expected.mean))
        {
            throw std::runtime_error("Worker result failed validation");
        }
    }
}

int main()
{
    try
    {
        const auto samples = generate_samples(26);
        const auto blocks = split_blocks(samples, 4);

        if (!blocks.empty() && blocks.size() - 1 > std::numeric_limits<std::uint32_t>::max())
        {
            throw std::runtime_error("Too many blocks for task IDs");
        }

        std::vector<Statistics> expected;
        expected.reserve(blocks.size());

        for (const auto block : blocks)
        {
            expected.push_back(calculate_statistics(block));
        }

        std::vector<std::optional<Statistics>> results(blocks.size());

        auto listener = listen_local(5000);

        std::cout << "Listening on 127.0.0.1:5000" << std::endl;

        auto worker = accept_worker(listener.get());

        std::size_t completed_tasks = 0;
        std::size_t processed_count = 0;
        std::size_t batch_start = 0;

        while (batch_start < blocks.size())
        {
            // batch size is the minimum of the max tasks per batch and the remaining blocks
            const std::size_t batch_count = std::min(
                protocol::max_tasks_per_batch,
                blocks.size() - batch_start);

            const std::size_t batch_end = batch_start + batch_count;

            const protocol::Message announcement{
                protocol::MessageType::batch_begin,
                0,
                {static_cast<std::uint8_t>(batch_count)}};

            protocol::send_message(worker.get(), announcement);

            for (std::size_t index = batch_start;
                 index < batch_end;
                 ++index)
            {
                const protocol::Message task{
                    protocol::MessageType::task,
                    static_cast<std::uint32_t>(index),
                    protocol::encode_task(blocks[index])};

                protocol::send_message(worker.get(), task);
            }

            // receive results for the current batch
            for (std::size_t index = 0; index < batch_count; ++index)
            {
                const auto reply = protocol::receive_message(worker.get());

                if (!reply)
                {
                    throw std::runtime_error(
                        "Worker closed before completing batch");
                }

                if (reply->type != protocol::MessageType::result)
                {
                    throw std::runtime_error("Expected result message");
                }

                const auto task_id =
                    static_cast<std::size_t>(reply->task_id);

                if (task_id < batch_start || task_id >= batch_end)
                {
                    throw std::runtime_error(
                        "Result task ID is outside the current batch");
                }

                if (results[task_id].has_value())
                {
                    throw std::runtime_error("Duplicate result");
                }

                const auto actual =
                    protocol::decode_result(reply->payload);

                validate_result(actual, expected[task_id]);

                results[task_id] = actual;
                processed_count += actual.count;
                ++completed_tasks;

                std::cout << "Verified task " << task_id
                          << ": count=" << actual.count
                          << ", mean=" << actual.mean
                          << '\n';
            }

            batch_start = batch_end;
        }

        if (completed_tasks != blocks.size() || processed_count != samples.size())
        {
            throw std::runtime_error("Incomplete result collection");
        }

        worker.reset();

        std::cout << "Completed " << completed_tasks
                  << " tasks covering " << processed_count
                  << " readings\n";

        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "Coordinator error: " << error.what() << '\n';
        return 1;
    }
}