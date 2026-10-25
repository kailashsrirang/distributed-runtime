#include "dataset.hpp"

#include <stdexcept>

// Generates a vector of samples with values ranging from -10.0 to 10.0 in increments of 0.1.
std::vector<double> generate_samples(std::size_t count)
{
    std::vector<double> samples;
    samples.reserve(count);

    for (std::size_t index = 0; index < count; ++index)
    {
        const auto position = static_cast<int>(index % 201);
        const double value = static_cast<double>(position - 100) / 10.0;

        samples.push_back(value);
    }

    return samples;
}

std::vector<std::span<const double>> split_blocks(std::span<const double> samples, std::size_t block_size)
{
    if (block_size == 0)
    {
        throw std::invalid_argument("block_size must be greater than zero");
    }

    std::vector<std::span<const double>> blocks;

    std::size_t offset = 0;

    while (offset < samples.size())
    {
        const std::size_t remaining = samples.size() - offset;

        const std::size_t count = remaining < block_size ? remaining : block_size;

        blocks.push_back(samples.subspan(offset, count));
        offset += count;
    }

    return blocks;
}