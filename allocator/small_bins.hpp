#ifndef SMALL_BINS_HPP
#define SMALL_BINS_HPP

#include <array>
#include <cstddef>
#include <tuple>
#include <cassert>
#include <vector>
#include <cmath>
#include "common.hpp"
#include "chunk_list.hpp"
#include "allocator_exception.hpp"

class SmallBins {
public:
    static constexpr size_t gap_between_bins = 8;
    static constexpr size_t bin_count = 5;
    static constexpr size_t min_chunk_size_for_bins = 16;
    static constexpr size_t max_chunk_size_for_bins = min_chunk_size_for_bins + (bin_count * gap_between_bins);

    /**
     * Initializes some small bins with some chunks within the address range given as parameters.
     * Note that during runtime, the range of small bins may change - given address range is
     * only for initialization.
     * SmallBins is not required to fill the whole region with some chunks. There may be a pending
     * chunk.
     * @param start_addr
     * @param end_addr
     */
    SmallBins()
    {
        initialize_bins();
    }

    /**
     * Initializes small bins in given memory region.
     * @param start_addr
     * @param end_addr Highest address.
     * @return Real address at which the initialization ended, it may be different than end_addr.
     */
    intptr_t initialize_memory(intptr_t start_addr, intptr_t end_addr)
    {
        assert(is_aligned(start_addr));
        assert(is_aligned(end_addr));

        std::array<ChunkList, bin_count> initial_chunk_lists;
        intptr_t last_addr = start_addr;
        bool fits_in_memory = fits_in_memory_region(start_addr, bins[0].chunk_sizes, end_addr);

        while (fits_in_memory) {
            for (size_t i = 0; i < bin_count; i++) {
                size_t chunk_size = bins[i].chunk_sizes;

                if (!fits_in_memory_region(start_addr, chunk_size, end_addr)) {
                    fits_in_memory = false;
                    break;
                }
                chunk_t *new_chunk = initialize_chunk(start_addr, chunk_size);
                initial_chunk_lists[i].prepend_chunk(new_chunk);

                start_addr += get_chunk_size(new_chunk);
                last_addr = start_addr;
            }
        }

        for (size_t i = 0; i < bins.size(); i++) {
            bins[i].chunk_list = initial_chunk_lists[i];
        }

        return last_addr;
    }

    /**
     * Allocates a chunk with exactly payload size. If it is not possible nullptr is returned.
     * @param payload_size Chunk's payload size.
     * @return May return nullptr.
     */
    chunk_t * allocate_chunk(size_t payload_size)
    {
        assert(contains_bin_with_chunk_size(payload_size));

        bin_t &bin = get_bin_with_chunk_size(payload_size);
        chunk_t *free_chunk = bin.chunk_list.find_free_chunk();
        if (free_chunk) {
            ChunkList::remove_chunk_from_list(free_chunk);
            return free_chunk;
        }
        else {
            return allocate_in_bin_with_higher_chunk_size(payload_size);
        }
    }

    /**
     * Adds a free chunk. Preferably is a situation, when last allocation failed, so it is
     * obvious that small bins needs some free chunk.
     * @param chunk Chunk with arbitrary size. Will be moved inside small bins. May also be
     * split.
     * @return List of residue chunks that were created during splitting of given chunk.
     */
    ChunkList add_chunk(chunk_t *chunk)
    {
        assert(chunk);

        return disperse_chunk_into_all_bins(chunk);
    }

    /// Returns bool whether given size fits in some small bin.
    bool contains_bin_with_chunk_size(size_t payload_size) const
    {
        for (auto &&bin : bins) {
            if (bin.chunk_sizes == payload_size) {
                return true;
            }
        }
        return false;
    }

private:
    struct bin_t {
        size_t chunk_sizes;
        ChunkList chunk_list;
    };
    std::array<bin_t, bin_count> bins;


    void initialize_bins()
    {
        for (size_t i = 0; i < bins.size(); ++i) {
            size_t chunk_size = min_chunk_size_for_bins + i * gap_between_bins;
            bins[i] = bin_t{chunk_size, ChunkList{}};
        }
    }

    chunk_t * allocate_in_bin_with_higher_chunk_size(size_t payload_size)
    {
        chunk_t *new_chunk = nullptr;
        chunk_t *bigger_free_chunk = find_smallest_free_chunk(payload_size);
        if (bigger_free_chunk) {
            if (is_chunk_splittable(bigger_free_chunk, payload_size)) {
                new_chunk = split_chunk(bigger_free_chunk, payload_size);
                move_chunk_to_correct_bin(bigger_free_chunk);
                // No need to move also new_chunk to correct bin, since it will be immediately used by user anyway.
            }
            else {
                new_chunk = bigger_free_chunk;
            }
        }

        return new_chunk;
    }

    ChunkList disperse_chunk_into_all_bins(chunk_t *chunk)
    {
        // TODO
    }

    void move_chunk_to_correct_bin(chunk_t *chunk)
    {
        assert(chunk);
        assert(contains_bin_with_chunk_size(chunk->payload_size));

        bin_t &bin = get_bin_with_chunk_size(chunk->payload_size);
        bin.chunk_list.prepend_chunk(chunk);
    }

    /// Finds first free chunk with size greater than payload_size parameter.
    /// If there is no such chunk, nullptr is returned.
    chunk_t * find_smallest_free_chunk(size_t payload_size)
    {
        for (const bin_t &bin : bins) {
            if (bin.chunk_sizes > payload_size) {
                chunk_t *free_chunk = bin.chunk_list.find_free_chunk();
                if (free_chunk) {
                    ChunkList::remove_chunk_from_list(free_chunk);
                    return free_chunk;
                }
            }
        }
        return nullptr;
    }

    bin_t & get_bin_with_chunk_size(size_t chunk_size)
    {
        for (auto &&bin : bins) {
            if (bin.chunk_sizes == chunk_size) {
                return bin;
            }
        }
    }
};


#endif //SMALL_BINS_HPP
