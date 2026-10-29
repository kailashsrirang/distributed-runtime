#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

// | Byte offset | Size    | Field                        |
// |------------ |---------|------------------------------|
// | 0           | 1 byte  | Protocol version             |
// | 1           | 1 byte  | Message type: task or result |
// | 2–3         | 2 bytes | Reserved; must be zero       |
// | 4–7         | 4 bytes | Task ID                      |
// | 8–11        | 4 bytes | Payload length in bytes      |

namespace protocol
{
    inline constexpr std::uint8_t version = 1;
    inline constexpr std::size_t header_size = 12;
    inline constexpr std::uint32_t max_payload_size = 64U * 1024U;
    inline constexpr std::size_t max_tasks_per_batch = 4;

    enum class MessageType : std::uint8_t
    {
        task = 1,
        result = 2,
        batch_begin = 3
    };

    struct Header
    {
        MessageType type;
        std::uint32_t task_id;
        std::uint32_t payload_size;
    };
    // type alias, type HeaderBytes: array containing uint8_t of length header_size
    using HeaderBytes = std::array<std::uint8_t, header_size>;

    HeaderBytes encode_header(const Header &header);

    Header decode_header(std::span<const std::uint8_t> bytes);
}