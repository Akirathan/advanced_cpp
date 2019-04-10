#ifndef LARGE_BIN_HPP
#define LARGE_BIN_HPP

#include <array>
#include <cstddef>
#include <tuple>
#include <cassert>
#include <vector>
#include <cmath>

/**
 * Acts as unsorted bin.
 */
class LargeBin {
public:
    /// Has to be multiple of alignment (8). Also has to be higher than SmallBins::max_chunk_size_for_bins.
    static constexpr size_t min_chunk_size = 256;
    /// Has to be multiple of alignment (8).
    static constexpr size_t initial_gap_between_chunks_sizes = 8 * 6;

    /// This method has same semantics as SmallBins.initialize_memory.
    intptr_t initialize_memory(intptr_t start_addr, intptr_t end_addr)
    {
        assert(is_aligned(start_addr));
        assert(is_aligned(end_addr));

        intptr_t last_addr = start_addr;
        size_t chunk_size = min_chunk_size;
        while (fits_in_memory_region(start_addr, chunk_size, end_addr)) {
            chunk_t *new_chunk = initialize_chunk(start_addr, chunk_size);
            large_chunk_list.append_chunk(new_chunk);

            chunk_size += initial_gap_between_chunks_sizes;
            start_addr += get_chunk_size(new_chunk);
            last_addr = start_addr;
        }

        return last_addr;
    }

    const ChunkList & get_chunk_list() const
    {
        return large_chunk_list;
    }

    void store_chunk(chunk_t *chunk)
    {
        assert(chunk);

        large_chunk_list.append_chunk(chunk);
    }

    chunk_t * pop_first_chunk()
    {
        return large_chunk_list.pop_first_chunk();
    }

    /**
     * Finds a chunk which is greater than payload_size, splits it, if it is feasible, and removes
     * it from the list.
     * @param payload_size Minimal size of the payload for new chunk.
     * @return May return nullptr if no such chunk is found.
     */
    chunk_t * pop_chunk_with_size_at_least(size_t payload_size)
    {
        chunk_t *large_chunk = large_chunk_list.pop_chunk_with_size_at_least(payload_size);
        if (!large_chunk) {
            return nullptr;
        }

        chunk_t *chunk_to_return = large_chunk;
        if (is_chunk_splittable(large_chunk, payload_size)) {
            chunk_to_return = split_chunk(large_chunk, payload_size);
        }

        return chunk_to_return;
    }

private:
    ChunkList large_chunk_list;
};


#endif //LARGE_BIN_HPP
