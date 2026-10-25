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
        constexpr std::uint8_t expected_request = 0x01;
        constexpr std::uint8_t reply = 0x02;

        auto coordinator = connect_local(port);

        std::cout << "Connected to coordinator\n";

        const auto request = receive_byte(coordinator.get());

        if (request != expected_request)
        {
            throw std::runtime_error("Unexpected coordinator request");
        }

        send_byte(coordinator.get(), reply);

        std::cout << "Reply sent\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "Worker error: " << error.what() << '\n';
        return 1;
    }
}