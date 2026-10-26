#include "message.hpp"

#include "tcp.hpp"

#include <stdexcept>

namespace protocol
{
    void send_message(int fd, const Message &message)
    {
        if (message.payload.size() > max_payload_size)
        {
            throw std::invalid_argument("Payload exceeds size limit");
        }

        const Header header{
            message.type,
            message.task_id,
            static_cast<std::uint32_t>(message.payload.size())};

        const auto encoded = encode_header(header);

        send_all(fd, encoded);
        send_all(fd, message.payload);
    }

    std::optional<Message> receive_message(int fd)
    {
        HeaderBytes encoded{};

        if (!receive_exact(fd, encoded))
        {
            return std::nullopt;
        }

        const Header header = decode_header(encoded);

        Message message{
            header.type,
            header.task_id,
            std::vector<std::uint8_t>(header.payload_size)};

        if (!receive_exact(fd, message.payload))
        {
            throw std::runtime_error("Connection closed before message payload");
        }

        return message;
    }
}