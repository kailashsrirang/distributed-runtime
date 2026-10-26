#include "message.hpp"
#include "payload.hpp"
#include "statistics.hpp"
#include "tcp.hpp"

#include <exception>
#include <iostream>
#include <stdexcept>

int main()
{
    try
    {
        auto coordinator = connect_local(5000);

        const auto task = protocol::receive_message(coordinator.get());

        if (!task)
        {
            throw std::runtime_error("Coordinator closed before task");
        }

        if (task->type != protocol::MessageType::task)
        {
            throw std::runtime_error("Expected task message");
        }

        const auto samples = protocol::decode_task(task->payload);
        const auto statistics = calculate_statistics(samples);

        const protocol::Message result{
            protocol::MessageType::result,
            task->task_id,
            protocol::encode_result(statistics)};

        protocol::send_message(coordinator.get(), result);

        std::cout << "Processed task " << task->task_id
                  << " containing " << samples.size()
                  << " readings\n";

        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "Worker error: " << error.what() << '\n';
        return 1;
    }
}