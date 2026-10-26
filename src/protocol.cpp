#include "protocol.hpp"

#include <stdexcept>

namespace
{
    // Helper function to write a 32-bit unsigned integer to a byte array in big-endian order into HeaderBytes
    void write_u32(protocol::HeaderBytes &bytes, std::size_t offset, std::uint32_t value)
    {
        bytes[offset] = static_cast<std::uint8_t>((value >> 24) & 0xFFU);
        bytes[offset + 1] = static_cast<std::uint8_t>((value >> 16) & 0xFFU);
        bytes[offset + 2] = static_cast<std::uint8_t>((value >> 8) & 0xFFU);
        bytes[offset + 3] = static_cast<std::uint8_t>(value & 0xFFU);
    }

    // Helper function to read a 32-bit unsigned integer from a byte array in big-endian order from HeaderBytes
    // user span to decouple code. can be used with other types of byte arrays
    std::uint32_t read_u32(std::span<const std::uint8_t> bytes, std::size_t offset)
    {
        return (static_cast<std::uint32_t>(bytes[offset]) << 24) | (static_cast<std::uint32_t>(bytes[offset + 1]) << 16) | (static_cast<std::uint32_t>(bytes[offset + 2]) << 8) | static_cast<std::uint32_t>(bytes[offset + 3]);
    }

    void validate(const protocol::Header &header)
    {
        if (header.type != protocol::MessageType::task && header.type != protocol::MessageType::result)
        {
            throw std::invalid_argument("Unknown message type");
        }

        if (header.payload_size > protocol::max_payload_size)
        {
            throw std::invalid_argument("Payload exceeds size limit");
        }
    }
}

namespace protocol
{
    HeaderBytes encode_header(const Header &header)
    {
        validate(header);

        HeaderBytes bytes{};

        bytes[0] = version;
        bytes[1] = static_cast<std::uint8_t>(header.type);

        write_u32(bytes, 4, header.task_id);
        write_u32(bytes, 8, header.payload_size);

        return bytes;
    }

    Header decode_header(std::span<const std::uint8_t> bytes)
    {
        if (bytes.size() != header_size)
        {
            throw std::invalid_argument("Incorrect header size");
        }

        if (bytes[0] != version)
        {
            throw std::invalid_argument("Unsupported protocol version");
        }

        if (bytes[2] != 0 || bytes[3] != 0)
        {
            throw std::invalid_argument("Reserved header bytes must be zero");
        }

        const Header header{
            static_cast<MessageType>(bytes[1]),
            read_u32(bytes, 4),
            read_u32(bytes, 8)};

        validate(header);

        return header;
    }
}