#include "message.hpp"
#include "tcp.hpp"

#include <exception>
#include <iostream>
#include <stdexcept>

int main()
{
    try
    {
        auto coordinator = connect_local(5000);

        // return Message type
        auto request = protocol::receive_message(coordinator.get());

        if (!request)
        {
            throw std::runtime_error("Coordinator closed before request");
        }

        if (request->type != protocol::MessageType::task)
        {
            throw std::runtime_error("Expected task message");
        }

        request->type = protocol::MessageType::result;

        protocol::send_message(coordinator.get(), *request);

        std::cout << "Replied to task " << request->task_id << '\n';

        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "Worker error: " << error.what() << '\n';
        return 1;
    }
}