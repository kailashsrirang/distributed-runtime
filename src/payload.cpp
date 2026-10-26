#include "payload.hpp"

#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>

static_assert(sizeof(double) == 8);
static_assert(std::numeric_limits<double>::is_iec559);

namespace
{
    // Append variable sized integer to a byte vector in big-endian order based on width.
    void append_integer(std::vector<std::uint8_t> &bytes, std::uint64_t value, unsigned int width)
    {
        for (unsigned int remaining = width; remaining > 0; --remaining)
        {
            const unsigned int shift = (remaining - 1) * 8;
            bytes.push_back(static_cast<std::uint8_t>((value >> shift) & 0xFFU));
        }
    }

    std::uint64_t read_integer(std::span<const std::uint8_t> bytes, std::size_t offset, unsigned int width)
    {
        std::uint64_t value = 0;

        for (unsigned int index = 0; index < width; ++index)
        {
            value = (value << 8) | bytes[offset + index];
        }
        return value;
    }

    void append_double(std::vector<std::uint8_t> &bytes, double value)
    {
        // check if floating point value is finite
        if (!std::isfinite(value))
        {
            throw std::invalid_argument("Non-finite floating-point value");
        }
        // cast value to double using raw 64-bit integer bit pattern
        append_integer(bytes, std::bit_cast<std::uint64_t>(value), 8);
    }

    double read_double(std::span<const std::uint8_t> bytes, std::size_t offset)
    {
        const double value = std::bit_cast<double>(read_integer(bytes, offset, 8));

        if (!std::isfinite(value))
        {
            throw std::invalid_argument("Non-finite floating-point value");
        }

        return value;
    }

    void validate_count(std::uint64_t count)
    {
        if (count == 0 || count > protocol::max_samples_per_task)
        {
            throw std::invalid_argument("Invalid sample count");
        }
    }
}

namespace protocol
{
    std::vector<std::uint8_t> encode_task(std::span<const double> samples)
    {
        validate_count(samples.size());
        std::vector<std::uint8_t> bytes;
        bytes.reserve(4 + samples.size() * 8);

        // write payload size(count) to the first 4 bytes of the payload
        append_integer(bytes, samples.size(), 4);

        // write each sample to the payload
        for (const double sample : samples)
        {
            append_double(bytes, sample);
        }
        return bytes;
    }

    std::vector<double> decode_task(std::span<const std::uint8_t> bytes)
    {
        if (bytes.size() < 4)
        {
            throw std::invalid_argument("Task payload is missing its count");
        }

        const auto count = read_integer(bytes, 0, 4);
        validate_count(count);

        const auto sample_count = static_cast<std::size_t>(count);

        if (bytes.size() != 4 + sample_count * 8)
        {
            throw std::invalid_argument("Task payload length mismatch");
        }

        std::vector<double> samples;
        samples.reserve(sample_count);

        for (std::size_t index = 0; index < sample_count; ++index)
        {
            samples.push_back(read_double(bytes, 4 + index * 8));
        }

        return samples;
    }

    std::vector<std::uint8_t> encode_result(const Statistics &result)
    {
        validate_count(result.count);

        if (result.minimum > result.maximum)
        {
            throw std::invalid_argument("Minimum exceeds maximum");
        }

        std::vector<std::uint8_t> bytes;
        bytes.reserve(28);

        append_integer(bytes, result.count, 4);
        append_double(bytes, result.minimum);
        append_double(bytes, result.maximum);
        append_double(bytes, result.mean);

        return bytes;
    }

    Statistics decode_result(std::span<const std::uint8_t> bytes)
    {
        if (bytes.size() != 28)
        {
            throw std::invalid_argument("Result payload must contain 28 bytes");
        }

        const auto count = read_integer(bytes, 0, 4);
        validate_count(count);

        const Statistics result{
            static_cast<std::size_t>(count),
            read_double(bytes, 4),
            read_double(bytes, 12),
            read_double(bytes, 20)};

        if (result.minimum > result.maximum)
        {
            throw std::invalid_argument("Minimum exceeds maximum");
        }

        return result;
    }
}