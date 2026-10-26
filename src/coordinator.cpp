#include "dataset.hpp"
#include "message.hpp"
#include "payload.hpp"
#include "statistics.hpp"
#include "tcp.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
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
        const auto samples = generate_samples(10);
        const auto blocks = split_blocks(samples, 4);

        std::vector<Statistics> expected;
        expected.reserve(blocks.size());

        for (const auto block : blocks)
        {
            expected.push_back(calculate_statistics(block));
        }

        auto listener = listen_local(5000);

        std::cout << "Listening on 127.0.0.1:5000" << std::endl;

        auto worker = accept_worker(listener.get());

        std::vector<Statistics> results;
        results.reserve(blocks.size());

        std::size_t processed_count = 0;

        for (std::size_t index = 0; index < blocks.size(); ++index)
        {
            if (index > std::numeric_limits<std::uint32_t>::max())
            {
                throw std::runtime_error("Too many blocks for task IDs");
            }

            const auto task_id = static_cast<std::uint32_t>(index);

            const protocol::Message task{
                protocol::MessageType::task,
                task_id,
                protocol::encode_task(blocks[index])};

            protocol::send_message(worker.get(), task);

            const auto reply =
                protocol::receive_message(worker.get());

            if (!reply)
            {
                throw std::runtime_error(
                    "Worker closed before returning all results");
            }

            if (reply->type != protocol::MessageType::result || reply->task_id != task_id)
            {
                throw std::runtime_error(
                    "Unexpected result type or task ID");
            }

            const auto actual =
                protocol::decode_result(reply->payload);

            validate_result(actual, expected[index]);

            results.push_back(actual);
            processed_count += actual.count;

            std::cout << "Verified task " << task_id
                      << ": count=" << actual.count
                      << ", mean=" << actual.mean
                      << '\n';
        }

        if (results.size() != blocks.size() || processed_count != samples.size())
        {
            throw std::runtime_error("Incomplete result collection");
        }

        worker.reset();

        std::cout << "Completed " << results.size()
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