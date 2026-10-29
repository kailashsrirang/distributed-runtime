#include "protocol.hpp"

#include <iostream>
#include <stdexcept>

namespace
{
    void require(bool condition, const char *message)
    {
        if (!condition)
        {
            throw std::runtime_error(message);
        }
    }

    void require_rejected(std::span<const std::uint8_t> bytes)
    {
        bool rejected = false;

        try
        {
            protocol::decode_header(bytes);
        }
        catch (const std::invalid_argument &)
        {
            rejected = true;
        }

        require(rejected, "Invalid header was accepted");
    }
}

int main()
{
    try
    {
        const protocol::Header original{protocol::MessageType::task, 0x12345678U, 256};

        const auto encoded = protocol::encode_header(original);

        const protocol::HeaderBytes expected{
            0x01, 0x01, 0x00, 0x00,
            0x12, 0x34, 0x56, 0x78,
            0x00, 0x00, 0x01, 0x00};

        require(encoded == expected, "Incorrect wire representation");

        const auto decoded = protocol::decode_header(expected);

        require(decoded.type == original.type, "Incorrect message type");
        require(decoded.task_id == original.task_id, "Incorrect task ID");
        require(decoded.payload_size == original.payload_size, "Incorrect payload size");

        auto invalid = expected;
        invalid[0] = 99;
        require_rejected(invalid);

        invalid = expected;
        invalid[1] = 99;
        require_rejected(invalid);

        invalid = expected;
        invalid[2] = 1;
        require_rejected(invalid);

        invalid = expected;
        invalid[3] = 1;
        require_rejected(invalid);

        invalid = expected;
        invalid[8] = 0x00;
        invalid[9] = 0x01;
        invalid[10] = 0x00;
        invalid[11] = 0x01;
        require_rejected(invalid);

        // pass in header with fewer than 12 bytes to ensure it is rejected
        require_rejected(std::span<const std::uint8_t>(expected).first(11));

        // pass in header with more than 12 bytes to ensure it is rejected
        const std::array<std::uint8_t, 13> too_long{};
        require_rejected(too_long);

        const auto empty_payload = protocol::decode_header(protocol::encode_header({protocol::MessageType::result, 0, 0}));

        require(empty_payload.payload_size == 0,
                "Zero-length payload should be allowed");

        const auto maximum_payload = protocol::decode_header(protocol::encode_header({protocol::MessageType::result, 42, protocol::max_payload_size}));

        require(maximum_payload.payload_size == protocol::max_payload_size,
                "Maximum payload size should be allowed");

        bool oversized_encode_rejected = false;

        try
        {
            protocol::encode_header({protocol::MessageType::task,
                                     0,
                                     protocol::max_payload_size + 1});

            const auto batch_header = protocol::decode_header(
                protocol::encode_header({protocol::MessageType::batch_begin,
                                         0,
                                         1}));

            require(
                batch_header.type == protocol::MessageType::batch_begin,
                "Batch message type was not preserved");
        }
        catch (const std::invalid_argument &)
        {
            oversized_encode_rejected = true;
        }

        require(oversized_encode_rejected,
                "Encoder accepted an oversized payload");

        std::cout << "All protocol tests passed\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "Test failed: " << error.what() << '\n';
        return 1;
    }
}