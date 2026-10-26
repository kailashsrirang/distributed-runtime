#pragma once

#include "protocol.hpp"

#include <optional>
#include <vector>

namespace protocol
{
    struct Message
    {
        MessageType type;
        std::uint32_t task_id;
        std::vector<std::uint8_t> payload;
    };

    void send_message(int fd, const Message &message);

    // use no return type to indicate that a connection was closed
    std::optional<Message> receive_message(int fd);
}