#include "message.hpp"
#include "processing_pool.hpp"
#include "protocol.hpp"
#include "tcp.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>

int main()
{
    try
    {
        auto coordinator = connect_local(5000);

        constexpr std::size_t thread_count = 2;

        ProcessingPool pool(thread_count, protocol::max_tasks_per_batch);

        std::size_t completed_tasks = 0;

        // worker completely drains a batch before requesting the next batch, so we can just loop until the coordinator closes the connection
        while (true)
        {
            // receive a batch announcement from the coordinator
            const auto batch = protocol::receive_message(coordinator.get());

            if (!batch)
            {
                break;
            }

            if (batch->type != protocol::MessageType::batch_begin || batch->task_id != 0 || batch->payload.size() != 1)
            {
                throw std::runtime_error("Invalid batch announcement");
            }

            const std::size_t batch_count = batch->payload[0];

            if (batch_count == 0 || batch_count > protocol::max_tasks_per_batch)
            {
                throw std::runtime_error("Invalid batch size");
            }

            // receive the tasks in the batch and submit them to the pool
            std::vector<std::uint32_t> task_ids;
            task_ids.reserve(batch_count);

            for (std::size_t index = 0; index < batch_count; ++index)
            {
                auto task = protocol::receive_message(coordinator.get());

                if (!task)
                {
                    throw std::runtime_error("Coordinator closed during a batch");
                }

                if (task->type != protocol::MessageType::task)
                {
                    throw std::runtime_error("Expected task message");
                }

                // check for duplicate task IDs within the task ID list
                if (std::find(task_ids.begin(), task_ids.end(), task->task_id) != task_ids.end())
                {
                    throw std::runtime_error("Duplicate task ID within batch");
                }

                // add task id to batch
                task_ids.push_back(task->task_id);

                // submit the task to the processing pool
                if (!pool.submit(std::move(*task)))
                {
                    throw std::runtime_error("Processing pool rejected a task");
                }
            }

            for (std::size_t index = 0; index < batch_count; ++index)
            {
                const auto result = pool.receive();

                if (!result)
                {
                    throw std::runtime_error(
                        "Processing pool stopped before completing batch");
                }

                protocol::send_message(coordinator.get(), *result);
                ++completed_tasks;

                std::cout << "Returned task "
                          << result->task_id << '\n';
            }
        }

        pool.finish();

        if (pool.receive())
        {
            throw std::runtime_error("Unexpected leftover pool result");
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