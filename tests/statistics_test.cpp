#include "statistics.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace
{
    bool nearly_equal(double actual, double expected)
    {
        constexpr double absolute_tolerance = 1e-12;
        constexpr double relative_tolerance = 1e-12;

        const double scale =
            std::max(std::abs(actual), std::abs(expected));

        return std::abs(actual - expected) <= absolute_tolerance + relative_tolerance * scale;
    }

    void require(bool condition, const char *message)
    {
        if (!condition)
        {
            throw std::runtime_error(message);
        }
    }
}

int main()
{
    try
    {
        const std::vector<double> normal{2.0, 4.0, 6.0, 8.0};
        const auto normal_result = calculate_statistics(normal);

        require(normal_result.count == 4, "Incorrect count");
        require(normal_result.minimum == 2.0, "Incorrect minimum");
        require(normal_result.maximum == 8.0, "Incorrect maximum");
        require(nearly_equal(normal_result.mean, 5.0), "Incorrect mean");

        const std::vector<double> single{-3.0};
        const auto single_result = calculate_statistics(single);

        require(single_result.count == 1, "Incorrect single count");
        require(single_result.minimum == -3.0, "Incorrect single minimum");
        require(single_result.maximum == -3.0, "Incorrect single maximum");
        require(nearly_equal(single_result.mean, -3.0),
                "Incorrect single mean");

        const std::vector<double> negative{-8.0, -2.0, -5.0};
        const auto negative_result = calculate_statistics(negative);

        require(negative_result.minimum == -8.0,
                "Incorrect negative minimum");
        require(negative_result.maximum == -2.0,
                "Incorrect negative maximum");
        require(nearly_equal(negative_result.mean, -5.0),
                "Incorrect negative mean");

        const std::vector<double> fractional{0.1, 0.2, 0.3};
        const auto fractional_result = calculate_statistics(fractional);

        require(nearly_equal(fractional_result.mean, 0.2),
                "Incorrect fractional mean");

        bool rejected_empty = false;

        try
        {
            calculate_statistics(std::span<const double>{});
        }
        catch (const std::invalid_argument &)
        {
            rejected_empty = true;
        }

        require(rejected_empty, "Empty input was not rejected");

        std::cout << "All statistics tests passed\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "Test failed: " << error.what() << '\n';
        return 1;
    }
}