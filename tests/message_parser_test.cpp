#include "message_parser.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace
{
    void require(bool condition, const char *message)
    {
        if (!condition)
        {
            throw std::runtime_error(message);
        }
    }

    std::vector<std::uint8_t> frame(const protocol::Message &message)
    {
        if (message.payload.size() > protocol::max_payload_size)
        {
            throw std::invalid_argument("Test payload is too large");
        }

        const auto header = protocol::encode_header({message.type,
                                                     message.task_id,
                                                     static_cast<std::uint32_t>(message.payload.size())});

        std::vector<std::uint8_t> bytes(header.begin(), header.end());

        bytes.insert(
            bytes.end(),
            message.payload.begin(),
            message.payload.end());

        return bytes;
    }

    void feed(
        protocol::MessageParser &parser,
        std::span<const std::uint8_t> input,
        std::vector<protocol::Message> &messages)
    {
        while (!input.empty())
        {
            const auto previous_size = input.size();

            auto message = parser.consume(input);

            require(
                input.size() < previous_size,
                "Parser failed to consume available input");

            if (message)
            {
                messages.push_back(std::move(*message));
            }
        }
    }

    void require_equal(
        const protocol::Message &actual,
        const protocol::Message &expected)
    {
        require(actual.type == expected.type, "Incorrect message type");
        require(actual.task_id == expected.task_id, "Incorrect task ID");
        require(actual.payload == expected.payload, "Incorrect payload");
    }
}

int main()
{
    try
    {
        const protocol::Message original{
            protocol::MessageType::task,
            42,
            {10, 20, 30, 40, 50}};

        const auto encoded = frame(original);

        // Every possible split into two chunks.
        for (std::size_t split = 0; split <= encoded.size(); ++split)
        {
            protocol::MessageParser parser;
            std::vector<protocol::Message> messages;

            const std::span<const std::uint8_t> bytes(encoded);

            feed(parser, bytes.first(split), messages);

            if (split < encoded.size())
            {
                require(messages.empty(), "Message completed too early");
            }

            feed(parser, bytes.subspan(split), messages);
            parser.finish();

            require(messages.size() == 1, "Expected exactly one message");
            require_equal(messages.front(), original);
        }

        // One byte at a time.
        {
            protocol::MessageParser parser;
            std::vector<protocol::Message> messages;

            for (std::size_t index = 0; index < encoded.size(); ++index)
            {
                feed(
                    parser,
                    std::span<const std::uint8_t>(encoded).subspan(
                        index, 1),
                    messages);

                const std::size_t expected_count =
                    index + 1 == encoded.size() ? 1 : 0;

                require(messages.size() == expected_count,
                        "Incorrect byte-by-byte completion");
            }

            parser.finish();
            require_equal(messages.front(), original);
        }

        // Multiple messages in one chunk, including an empty payload.
        {
            const protocol::Message empty{
                protocol::MessageType::result,
                43,
                {}};

            auto combined = encoded;
            const auto empty_frame = frame(empty);

            combined.insert(
                combined.end(),
                empty_frame.begin(),
                empty_frame.end());

            combined.insert(
                combined.end(),
                encoded.begin(),
                encoded.end());

            protocol::MessageParser parser;
            std::vector<protocol::Message> messages;

            feed(parser, combined, messages);
            parser.finish();

            require(messages.size() == 3, "Expected three messages");
            require_equal(messages[0], original);
            require_equal(messages[1], empty);
            require_equal(messages[2], original);
        }

        // EOF at every incomplete position must be rejected.
        for (std::size_t length = 1; length < encoded.size(); ++length)
        {
            protocol::MessageParser parser;
            std::vector<protocol::Message> messages;

            feed(
                parser,
                std::span<const std::uint8_t>(encoded).first(length),
                messages);

            bool rejected = false;

            try
            {
                parser.finish();
            }
            catch (const std::runtime_error &)
            {
                rejected = true;
            }

            require(rejected, "Truncated message was accepted");
        }

        // Empty input and EOF between messages are valid.
        {
            protocol::MessageParser parser;
            std::span<const std::uint8_t> empty;

            require(!parser.consume(empty),
                    "Empty input produced a message");

            parser.finish();
        }

        // Oversized payload advertised in a complete header.
        {
            auto invalid = protocol::encode_header({protocol::MessageType::task,
                                                    0,
                                                    0});

            // Big-endian 65,537: one byte beyond the payload limit.
            invalid[8] = 0x00;
            invalid[9] = 0x01;
            invalid[10] = 0x00;
            invalid[11] = 0x01;

            protocol::MessageParser parser;
            std::span<const std::uint8_t> input(invalid);

            bool rejected = false;

            try
            {
                parser.consume(input);
            }
            catch (const std::invalid_argument &)
            {
                rejected = true;
            }

            require(rejected, "Oversized header was accepted");

            bool reuse_rejected = false;
            std::span<const std::uint8_t> empty;

            try
            {
                parser.consume(empty);
            }
            catch (const std::logic_error &)
            {
                reuse_rejected = true;
            }

            require(reuse_rejected, "Failed parser allowed reuse");
        }

        // Largest allowed payload, delivered in small chunks.
        {
            const protocol::Message maximum{
                protocol::MessageType::result,
                99,
                std::vector<std::uint8_t>(
                    protocol::max_payload_size,
                    0xAB)};

            const auto bytes = frame(maximum);

            protocol::MessageParser parser;
            std::vector<protocol::Message> messages;

            std::size_t offset = 0;

            while (offset < bytes.size())
            {
                const std::size_t count = std::min(
                    std::size_t{257},
                    bytes.size() - offset);

                feed(
                    parser,
                    std::span<const std::uint8_t>(bytes).subspan(
                        offset, count),
                    messages);

                offset += count;
            }

            parser.finish();

            require(messages.size() == 1,
                    "Maximum payload did not produce one message");

            require_equal(messages.front(), maximum);
        }

        std::cout << "All incremental parser tests passed\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "Test failed: " << error.what() << '\n';
        return 1;
    }
}