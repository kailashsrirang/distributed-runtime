#include "dataset.hpp"
#include "statistics.hpp"

#include <cstddef>
#include <iostream>

int main()
{
    const auto samples = generate_samples(10);
    const auto blocks = split_blocks(samples, 4);

    std::size_t processed_count = 0;

    for (std::size_t block_id = 0; block_id < blocks.size(); ++block_id)
    {
        const Statistics result = calculate_statistics(blocks[block_id]);

        processed_count += result.count;

        std::cout << "Block " << block_id
                  << ": count=" << result.count
                  << ", min=" << result.minimum
                  << ", max=" << result.maximum
                  << ", mean=" << result.mean
                  << '\n';
    }

    std::cout << "Processed " << processed_count
              << " readings across " << blocks.size()
              << " blocks\n";

    return 0;
}