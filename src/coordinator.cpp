#include "dataset.hpp"
#include "message.hpp"
#include "payload.hpp"
#include "statistics.hpp"
#include "tcp.hpp"

#include <algorithm>
#include <cmath>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <utility>

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
}

int main()
{
    try
    {
        const auto samples = generate_samples(10);
        const auto expected = calculate_statistics(samples);

        const protocol::Message task{
            protocol::MessageType::task,
            7,
            protocol::encode_task(samples)};

        auto listener = listen_local(5000);

        std::cout << "Listening on 127.0.0.1:5000" << std::endl;

        auto worker = accept_worker(listener.get());

        protocol::send_message(worker.get(), task);

        const auto reply = protocol::receive_message(worker.get());

        if (!reply)
        {
            throw std::runtime_error("Worker closed before replying");
        }

        if (reply->type != protocol::MessageType::result || reply->task_id != task.task_id)
        {
            throw std::runtime_error("Unexpected result type or task ID");
        }

        const auto actual = protocol::decode_result(reply->payload);

        if (actual.count != expected.count || actual.minimum != expected.minimum || actual.maximum != expected.maximum || !nearly_equal(actual.mean, expected.mean))
        {
            throw std::runtime_error("Worker result failed validation");
        }

        std::cout << "Verified task " << reply->task_id << '\n';
        std::cout << "Count: " << actual.count << '\n';
        std::cout << "Minimum: " << actual.minimum << '\n';
        std::cout << "Maximum: " << actual.maximum << '\n';
        std::cout << "Mean: " << actual.mean << '\n';

        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "Coordinator error: " << error.what() << '\n';
        return 1;
    }
}