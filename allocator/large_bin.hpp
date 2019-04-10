#ifndef LARGE_BIN_HPP
#define LARGE_BIN_HPP

#include <array>
#include <cstddef>
#include <tuple>
#include <cassert>
#include <vector>
#include <cmath>

class LargeBin {
public:
    /// Has to be multiple of alignment (8).
    static constexpr size_t min_chunk_size = 256;

    /// This method has same semantics as SmallBins.initialize_memory.
    intptr_t initialize_memory(intptr_t start_addr, intptr_t end_addr)
    {
        assert(is_aligned(start_addr));
        assert(is_aligned(end_addr));

        // TODO
    }

    /**
     * Finds a chunk which is greater than payload_size and removes it from the list.
     * @param payload_size
     * @return May return nullptr if no such chunk is found.
     */
    chunk_t * get_chunk_with_size_at_least(size_t payload_size)
    {
        // TODO
        return nullptr;
    }

private:
    chunk_t *chunks;
};


#endif //LARGE_BIN_HPP
