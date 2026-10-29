#include "message_parser.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace protocol
{
    std::optional<Message> MessageParser::consume(
        std::span<const std::uint8_t> &input)
    // pass input by reference so consume can modify caller's view of input. i.e. to show how much of input was consumed
    {
        if (failed_)
        {
            throw std::logic_error("Cannot reuse a failed message parser");
        }

        try
        {
            // if parser does not have a header, try to read the header first
            if (!header_)
            {
                // grab header bytes only or bytes that are available in input
                const std::size_t count = std::min(
                    header_size - header_used_,
                    input.size());

                if (count > 0)
                {
                    // copy form input to header_bytes_
                    std::copy_n(
                        input.data(),
                        count,
                        header_bytes_.data() + header_used_);

                    header_used_ += count;
                    input = input.subspan(count);
                }

                if (header_used_ < header_size)
                {
                    // not enough to complete the header, return nullopt and wait for more input
                    return std::nullopt;
                }

                header_ = decode_header(header_bytes_);

                // Header validation happens before allocating payload memory.
                payload_.resize(header_->payload_size);
                payload_used_ = 0;
            }

            // try to read payload
            const std::size_t count = std::min(
                payload_.size() - payload_used_,
                input.size());

            if (count > 0)
            {
                // copy from input to payload_
                std::copy_n(
                    input.data(),
                    count,
                    payload_.data() + payload_used_);

                payload_used_ += count;
                input = input.subspan(count);
            }

            if (payload_used_ < payload_.size())
            {
                // not enough to complete the payload, return nullopt and wait for more input
                return std::nullopt;
            }
            // assemble the completed message
            Message completed{
                header_->type,
                header_->task_id,
                std::move(payload_)};

            // reset parser state
            header_.reset();
            header_used_ = 0;
            payload_used_ = 0;
            payload_.clear();

            return completed;
        }
        catch (...)
        {
            failed_ = true;
            throw;
        }
    }

    void MessageParser::finish() const
    {
        if (failed_)
        {
            throw std::logic_error("Message parser is in a failed state");
        }

        if (header_used_ != 0 || header_.has_value())
        {
            throw std::runtime_error("Connection ended during a message");
        }
    }
}