#include "statistics.hpp"

#include <stdexcept>

Statistics calculate_statistics(std::span<const double> samples)
{
    if (samples.empty())
    {
        throw std::invalid_argument(
            "calculate_statistics requires at least one sample");
    }

    double minimum = samples.front();
    double maximum = samples.front();
    double sum = 0.0;

    for (const double sample : samples)
    {
        if (sample < minimum)
        {
            minimum = sample;
        }

        if (sample > maximum)
        {
            maximum = sample;
        }

        sum += sample;
    }

    return Statistics{
        samples.size(),
        minimum,
        maximum,
        sum / static_cast<double>(samples.size())};
}