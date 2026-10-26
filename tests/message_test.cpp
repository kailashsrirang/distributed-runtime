#include "message.hpp"
#include "tcp.hpp"

#include <sys/socket.h>

#include <array>
#include <iostream>
#include <stdexcept>
#include <utility>

namespace
{
    void require(bool condition, const char *message)
    {
        if (!condition)
        {
            throw std::runtime_error(message);
        }
    }

    std::array<UniqueFd, 2> connected_pair()
    {
        int fds[2];
        // create a pair of connected sockets for testing
        if (::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == -1)
        {
            throw std::runtime_error("socketpair failed");
        }

        return {UniqueFd(fds[0]), UniqueFd(fds[1])};
    }

    void require_receive_failure(int fd)
    {
        bool failed = false;

        try
        {
            protocol::receive_message(fd);
        }
        catch (const std::runtime_error &)
        {
            failed = true;
        }

        require(failed, "Truncated message was accepted");
    }
}

int main()
{
    try
    {
        {
            auto sockets = connected_pair();

            const protocol::Message first{protocol::MessageType::task, 7, {10, 20, 30}};

            const protocol::Message second{protocol::MessageType::result, 8, {}};

            protocol::send_message(sockets[0].get(), first);
            protocol::send_message(sockets[0].get(), second);
            sockets[0].reset(); // signal EOF to the receiver

            const auto received_first = protocol::receive_message(sockets[1].get());

            require(received_first.has_value(), "First message missing");
            require(received_first->type == first.type, "Incorrect first type");
            require(received_first->task_id == first.task_id, "Incorrect first task ID");
            require(received_first->payload == first.payload, "Incorrect first payload");

            const auto received_second = protocol::receive_message(sockets[1].get());

            require(received_second.has_value(), "Second message missing");
            require(received_second->type == second.type, "Incorrect second type");
            require(received_second->task_id == second.task_id, "Incorrect second task ID");
            require(received_second->payload.empty(), "Expected empty payload");

            require(!protocol::receive_message(sockets[1].get()), "Expected clean EOF");
        }

        {
            auto sockets = connected_pair();

            const auto header = protocol::encode_header({protocol::MessageType::task, 42, 3});

            for (const auto byte : header)
            {
                send_byte(sockets[0].get(), byte);
            }

            const std::array<std::uint8_t, 3> payload{1, 2, 3};

            for (const auto byte : payload)
            {
                send_byte(sockets[0].get(), byte);
            }

            sockets[0].reset(); // signal EOF to the receiver

            const auto received = protocol::receive_message(sockets[1].get());

            require(received.has_value(), "Split-write message missing");
            require(received->task_id == 42, "Incorrect split-write ID");
            require(received->payload == std::vector<std::uint8_t>{1, 2, 3}, "Incorrect split-write payload");
        }

        {
            auto sockets = connected_pair();

            const auto header = protocol::encode_header({protocol::MessageType::task, 0, 0});

            send_all(
                sockets[0].get(),
                std::span<const std::uint8_t>(header).first(5));

            sockets[0].reset();

            require_receive_failure(sockets[1].get());
        }

        {
            auto sockets = connected_pair();

            const auto header = protocol::encode_header({protocol::MessageType::task, 0, 3});

            send_all(sockets[0].get(), header);
            send_byte(sockets[0].get(), 99);
            sockets[0].reset();

            require_receive_failure(sockets[1].get());
        }

        std::cout << "All framed message tests passed\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "Test failed: " << error.what() << '\n';
        return 1;
    }
}