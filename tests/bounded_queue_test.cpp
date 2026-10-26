#include "bounded_queue.hpp"

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
}

int main()
{
    try
    {
        bool rejected_zero = false;

        try
        {
            BoundedQueue<int> invalid(0);
        }
        catch (const std::invalid_argument &)
        {
            rejected_zero = true;
        }

        require(rejected_zero, "Zero capacity was accepted");

        {
            BoundedQueue<int> queue(2);

            require(queue.push(10), "First push failed");
            require(queue.push(20), "Second push failed");

            queue.close();
            queue.close();

            require(!queue.push(30), "Closed queue accepted a push");

            const auto first = queue.pop();
            const auto second = queue.pop();
            const auto end = queue.pop();

            require(first && *first == 10, "Incorrect first item");
            require(second && *second == 20, "Incorrect second item");
            require(!end, "drained queue should return no item");
        }

        {
            BoundedQueue<int> queue(2);
            constexpr int item_count = 1000;

            auto producer = std::async(std::launch::async, [&]
                                       {
                try
                {
                    for (int value = 0; value < item_count; ++value)
                    {

                        if (!queue.push(value))
                        {
                            queue.close();
                            return false;
                        }
                    }

                    queue.close();
                    return true;
                }
                catch (...)
                {
                    // If the producer throws, 
                    //we need to close the queue to unblock the consumer
                    queue.close();
                    throw;
                } });

            std::vector<int> received;

            try
            {
                while (const auto item = queue.pop())
                {
                    received.push_back(*item);
                }
            }
            catch (...)
            {
                queue.close();
                throw;
            }

            require(producer.get(), "Producer stopped unexpectedly");

            require(received.size() == item_count,
                    "Items were lost or duplicated");

            for (int index = 0; index < item_count; ++index)
            {
                require(
                    received[static_cast<std::size_t>(index)] == index,
                    "FIFO order was not preserved");
            }
        }

        {
            BoundedQueue<int> queue(1);
            require(queue.push(1), "Initial push failed");

            auto pending_push = std::async(std::launch::async, [&]
                                           { return queue.push(2); });

            queue.close();

            const bool accepted = pending_push.get();
            const auto remaining = queue.pop();
            const auto end = queue.pop();

            require(!accepted, "Push into full closed queue succeeded");
            require(remaining && *remaining == 1,
                    "Close discarded the queued item");
            require(!end, "Closed queue did not drain");
        }

        {
            BoundedQueue<int> queue(1);

            auto pending_pop = std::async(std::launch::async, [&]
                                          { return queue.pop(); });

            queue.close();

            require(!pending_pop.get(),
                    "Empty closed queue returned an item");
        }

        std::cout << "All bounded queue tests passed\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "Test failed: " << error.what() << '\n';
        return 1;
    }
}