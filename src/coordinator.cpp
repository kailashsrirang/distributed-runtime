#include "message.hpp"
#include "tcp.hpp"

#include <exception>
#include <iostream>
#include <stdexcept>

int main()
{
    try
    {
        auto listener = listen_local(5000);

        std::cout << "Listening on 127.0.0.1:5000" << std::endl;

        auto worker = accept_worker(listener.get());

        const protocol::Message request{protocol::MessageType::task, 7, {10, 20, 30}};

        protocol::send_message(worker.get(), request);

        const auto reply = protocol::receive_message(worker.get());

        if (!reply)
        {
            throw std::runtime_error("Worker closed before replying");
        }

        if (reply->type != protocol::MessageType::result || reply->task_id != request.task_id || reply->payload != request.payload)
        {
            throw std::runtime_error("Incorrect worker reply");
        }

        std::cout << "Verified reply for task " << reply->task_id
                  << " with " << reply->payload.size()
                  << " payload bytes\n";

        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "Coordinator error: " << error.what() << '\n';
        return 1;
    }
}