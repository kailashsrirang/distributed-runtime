#pragma once

#include <cstddef>
#include <span>

struct Statistics
{
    std::size_t count;
    double minimum;
    double maximum;
    double mean;
};

Statistics calculate_statistics(std::span<const double> samples);