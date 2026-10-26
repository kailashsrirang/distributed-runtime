#include "message.hpp"
#include "payload.hpp"
#include "statistics.hpp"
#include "tcp.hpp"

#include <cstddef>
#include <exception>
#include <iostream>
#include <stdexcept>

int main()
{
    try
    {
        auto coordinator = connect_local(5000);

        std::size_t completed_tasks = 0;

        while (true)
        {
            const auto task =
                protocol::receive_message(coordinator.get());

            if (!task)
            {
                break;
            }

            if (task->type != protocol::MessageType::task)
            {
                throw std::runtime_error("Expected task message");
            }

            const auto samples =
                protocol::decode_task(task->payload);

            const auto statistics =
                calculate_statistics(samples);

            const protocol::Message result{
                protocol::MessageType::result,
                task->task_id,
                protocol::encode_result(statistics)};

            protocol::send_message(coordinator.get(), result);

            ++completed_tasks;

            std::cout << "Processed task " << task->task_id
                      << " containing " << samples.size()
                      << " readings\n";
        }

        std::cout << "Coordinator closed connection; completed "
                  << completed_tasks << " tasks\n";

        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "Worker error: " << error.what() << '\n';
        return 1;
    }
}