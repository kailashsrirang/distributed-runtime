#include "payload.hpp"

#include <iostream>
#include <limits>
#include <stdexcept>
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

    // accept any type as argument as long as it is callable with no arguments and returns void
    template <typename Function>
    void require_invalid(Function operation)
    {
        bool rejected = false;

        try
        {
            operation();
        }
        catch (const std::invalid_argument &)
        {
            rejected = true;
        }

        require(rejected, "Invalid payload was accepted");
    }
}

int main()
{
    try
    {
        const std::vector<double> one{1.0};

        const std::vector<std::uint8_t> expected{
            0x00, 0x00, 0x00, 0x01, // count
            0x3F, 0xF0, 0x00, 0x00, // 1.0 in IEEE 754 binary32 format
            0x00, 0x00, 0x00, 0x00};

        require(protocol::encode_task(one) == expected, "Incorrect task wire representation");

        require(protocol::decode_task(expected) == one, "Incorrect decoded task");

        const std::vector<double> samples{-2.5, 0.0, 3.25};

        require(protocol::decode_task(protocol::encode_task(samples)) == samples, "Task round trip changed readings");

        const Statistics original{3, -2.5, 3.25, 0.25};

        const auto result = protocol::decode_result(protocol::encode_result(original));

        require(result.count == original.count, "Incorrect result count");
        require(result.minimum == original.minimum, "Incorrect result minimum");
        require(result.maximum == original.maximum, "Incorrect result maximum");
        require(result.mean == original.mean, "Incorrect result mean");

        auto truncated = expected;
        truncated.pop_back();
        // [&] is a lambda that captures all local variables by reference, so we can use it to call decode_task with the truncated vector
        require_invalid([&]
                        { protocol::decode_task(truncated); });

        auto trailing = expected;
        trailing.push_back(0);

        // vector too big to be decoded
        require_invalid([&]
                        { protocol::decode_task(trailing); });

        // emtpy vector cannot be encoded
        require_invalid([]
                        { protocol::encode_task(std::span<const double>{}); });

        // count and buffer mismatch
        require_invalid([]
                        { protocol::decode_task(
                              std::vector<std::uint8_t>{0x00, 0x00, 0x20, 0x00}); });

        // vector too small to be decoded
        require_invalid([]
                        { protocol::decode_result(std::vector<std::uint8_t>(27)); });

        // cant pass non finite values to encode_task
        require_invalid([]
                        {
            const std::vector<double> values{
                std::numeric_limits<double>::infinity()
            };

            protocol::encode_task(values); });

        auto infinity = expected;
        infinity[4] = 0x7F; // set the first byte of the first double to 0x7F, which is the exponent for infinity in IEEE 754

        require_invalid([&]
                        { protocol::decode_task(infinity); });

        // test the maximum size of a task/max samples. each sample is set to 1.0
        const std::vector<double> maximum(protocol::max_samples_per_task, 1.0);

        require(protocol::decode_task(protocol::encode_task(maximum)) == maximum, "Maximum-size task failed");

        // max sample length exceeded
        require_invalid([]
                        {
            const std::vector<double> oversized(
                protocol::max_samples_per_task + 1,
                1.0
            );

            protocol::encode_task(oversized); });

        std::cout << "All payload tests passed\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "Test failed: " << error.what() << '\n';
        return 1;
    }
}