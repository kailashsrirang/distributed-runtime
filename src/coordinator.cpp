#include "tcp.hpp"

#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>

int main()
{
    try
    {
        constexpr std::uint16_t port = 5000;
        constexpr std::uint8_t request = 0x01;
        constexpr std::uint8_t expected_reply = 0x02;

        auto listener = listen_local(port);

        std::cout << "Listening on 127.0.0.1:" << port << std::endl;

        auto worker = accept_worker(listener.get());

        std::cout << "Worker connected\n";

        send_byte(worker.get(), request);

        const auto reply = receive_byte(worker.get());

        if (reply != expected_reply)
        {
            throw std::runtime_error("Unexpected worker reply");
        }

        std::cout << "Worker reply verified\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "Coordinator error: " << error.what() << '\n';
        return 1;
    }
}