#pragma once

#include "protocol.hpp"
#include "statistics.hpp"

#include <span>
#include <vector>

// | Payload | Format                                               |
// |---------|------------------------------------------------------|
// | Task    | 4-byte sample count, followed by 8 bytes per reading |
// | Result  | 4-byte count, then 8-byte minimum, maximum, and mean |

namespace protocol
{
    inline constexpr std::size_t max_samples_per_task = (max_payload_size - 4) / 8;
    std::vector<std::uint8_t> encode_task(std::span<const double> samples);
    std::vector<double> decode_task(std::span<const std::uint8_t> bytes);
    std::vector<std::uint8_t> encode_result(const Statistics &result);
    Statistics decode_result(std::span<const std::uint8_t> bytes);
}