#include "dataset.hpp"
#include "statistics.hpp"

#include <iostream>
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
}

int main()
{
    try
    {
        const auto generated = generate_samples(203);

        require(generated.size() == 203, "Incorrect generated size");
        require(generated.front() == -10.0, "Incorrect first value");
        require(generated[100] == 0.0, "Incorrect midpoint value");
        require(generated[200] == 10.0, "Incorrect upper endpoint");
        require(generated[201] == -10.0, "Sequence did not repeat");
        require(generated == generate_samples(203),
                "Generation is not repeatable");

        const std::vector<double> samples{
            1.0, 2.0, 3.0, 4.0, 5.0,
            6.0, 7.0, 8.0, 9.0, 10.0};

        const auto uneven = split_blocks(samples, 4);

        require(uneven.size() == 3, "Incorrect uneven block count");
        require(uneven[0].size() == 4, "Incorrect first block size");
        require(uneven[1].size() == 4, "Incorrect second block size");
        require(uneven[2].size() == 2, "Incorrect final block size");

        std::size_t next_index = 0;

        for (const auto block : uneven)
        {
            require(block.data() == samples.data() + next_index,
                    "Block does not reference the expected input");

            for (const double sample : block)
            {
                require(next_index < samples.size(),
                        "Blocks contain too many readings");
                require(sample == samples[next_index],
                        "Reading was lost, duplicated, or reordered");
                ++next_index;
            }
        }

        require(next_index == samples.size(), "Readings were omitted");

        const auto final_result = calculate_statistics(uneven.back());

        require(final_result.count == 2, "Incorrect final block count");
        require(final_result.minimum == 9.0, "Incorrect final minimum");
        require(final_result.maximum == 10.0, "Incorrect final maximum");
        require(final_result.mean == 9.5, "Incorrect final mean");

        const auto exact = split_blocks(samples, 5);

        require(exact.size() == 2, "Incorrect exact block count");
        require(exact[0].size() == 5 && exact[1].size() == 5,
                "Incorrect exact block sizes");

        const auto oversized = split_blocks(samples, 20);

        require(oversized.size() == 1, "Expected one oversized block");
        require(oversized[0].size() == samples.size(),
                "Oversized block lost readings");

        const auto singles = split_blocks(samples, 1);

        require(singles.size() == samples.size(),
                "Incorrect single-reading block count");

        for (const auto block : singles)
        {
            require(block.size() == 1, "Expected single-reading blocks");
        }

        const auto empty = generate_samples(0);

        require(empty.empty(), "Expected empty generated input");
        require(split_blocks(empty, 4).empty(),
                "Empty input should produce no blocks");

        bool rejected_zero = false;

        try
        {
            split_blocks(samples, 0);
        }
        catch (const std::invalid_argument &)
        {
            rejected_zero = true;
        }

        require(rejected_zero, "Zero block size was not rejected");

        std::cout << "All dataset tests passed\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "Test failed: " << error.what() << '\n';
        return 1;
    }
}