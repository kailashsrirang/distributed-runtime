#pragma once

#include <cstddef>
#include <span>
#include <vector>

std::vector<double> generate_samples(std::size_t count);

std::vector<std::span<const double>> split_blocks(
    std::span<const double> samples,
    std::size_t block_size);