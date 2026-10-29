#pragma once

#include "message.hpp"
#include "protocol.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace protocol
{
    class MessageParser
    {
    public:
        // Consumes bytes from the front of input.
        // Returns at most one completed message.
        // Unconsumed bytes remain in input.
        std::optional<Message> consume(
            std::span<const std::uint8_t> &input);

        // Called when the connection reaches EOF.
        // Throws if a message is incomplete.
        void finish() const;

    private:
        HeaderBytes header_bytes_{};
        std::size_t header_used_ = 0;

        std::optional<Header> header_;

        std::vector<std::uint8_t> payload_;
        std::size_t payload_used_ = 0;

        bool failed_ = false;
    };
}