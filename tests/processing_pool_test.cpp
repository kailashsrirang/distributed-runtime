#include "processing_pool.hpp"

#include "payload.hpp"

#include <cstddef>
#include <cstdint>
#include <future>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace
{
    void require(bool condition, const char *message)
    {
        if (!condition)
        {
            throw std::runtime_error(message);
        }
    }

    protocol::Message make_task(std::uint32_t id)
    {
        const double value = static_cast<double>(id);

        const std::vector<double> samples{
            value,
            value + 2.0};

        return protocol::Message{
            protocol::MessageType::task,
            id,
            protocol::encode_task(samples)};
    }
}

int main()
{
    try
    {
        bool rejected_zero = false;

        try
        {
            ProcessingPool invalid(0, 2);
        }
        catch (const std::invalid_argument &)
        {
            rejected_zero = true;
        }

        require(rejected_zero, "Zero threads were accepted");

        {
            ProcessingPool pool(2, 2);
            pool.finish();

            require(!pool.receive(), "Empty pool produced a result");
            require(!pool.submit(make_task(0)), "Finished pool accepted a task");
        }

        {
            constexpr std::uint32_t task_count = 50;

            ProcessingPool pool(3, 2);

            // Submission and result collection run concurrently so neither
            // bounded queue prevents the other side from making progress.
            auto producer = std::async(std::launch::async, [&]
                                       {
                try
                {
                    for (std::uint32_t id = 0; id < task_count; ++id)
                    {
                        if (!pool.submit(make_task(id)))
                        {
                            throw std::runtime_error(
                                "Task submission was rejected"
                            );
                        }
                    }
                    //no more task incoming => wake up all worker threads
                    pool.finish();
                }
                catch (...)
                {
                    //wake up all threads
                    pool.finish();
                    throw;
                } });

            std::vector<protocol::Message> received;

            try
            {
                while (auto result = pool.receive())
                {
                    received.push_back(std::move(*result));
                }
            }
            catch (...)
            {
                // Ensure pool shutdown occurs before waiting for a producer
                // that might otherwise remain blocked.
                pool.finish();
                throw;
            }

            producer.get();

            require(received.size() == task_count, "Incorrect result count");

            std::vector<bool> seen(task_count, false);

            for (const auto &message : received)
            {
                require(message.type == protocol::MessageType::result,
                        "Incorrect result message type");

                require(message.task_id < task_count,
                        "Invalid result task ID");

                require(!seen[message.task_id], "Duplicate task result");
                seen[message.task_id] = true;

                const auto result = protocol::decode_result(message.payload);

                const double value =
                    static_cast<double>(message.task_id);

                require(result.count == 2, "Incorrect sample count");
                require(result.minimum == value, "Incorrect minimum");
                require(result.maximum == value + 2.0,
                        "Incorrect maximum");
                require(result.mean == value + 1.0, "Incorrect mean");
            }
        }

        {
            ProcessingPool pool(2, 2);

            require(
                pool.submit({protocol::MessageType::task,
                             0,
                             {}}),
                "Malformed task was not submitted");

            pool.finish();

            bool failure_reported = false;

            try
            {
                while (pool.receive())
                {
                }
            }
            catch (const std::invalid_argument &)
            {
                failure_reported = true;
            }

            require(failure_reported,
                    "Processing failure was not reported");
        }

        {
            ProcessingPool pool(2, 2);

            pool.submit(make_task(0));
            pool.submit(make_task(1));

            // Destruction must finish even without collecting results
        }

        std::cout << "All processing pool tests passed\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "Test failed: " << error.what() << '\n';
        return 1;
    }
}