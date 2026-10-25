#include <iostream>
#include "statistics.hpp"

#include <vector>

int main()
{
    const std::vector<double> samples{1.0, 2.0, 3.0, 4.0, 5.0};

    const Statistics result = calculate_statistics(samples);

    std::cout << "Count: " << result.count << '\n';
    std::cout << "Minimum: " << result.minimum << '\n';
    std::cout << "Maximum: " << result.maximum << '\n';
    std::cout << "Mean: " << result.mean << '\n';
    return 0;
}