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
    static constexpr size_t gap_between_bins = alignment;
    /// Must not be too high - it if is high, it may happen that during split redundant
    /// chunk that wont fit into any bin will be created. And there will be no way to
    /// clean it up. In other words: if this number is too high, memory leaks may happen.
    static constexpr size_t min_chunk_size_for_bins = min_payload_size;
    static constexpr size_t max_chunk_size_for_bins = 512;
    static constexpr size_t bin_count = ((max_chunk_size_for_bins - min_chunk_size_for_bins) / gap_between_bins) + 1;

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
    address_t initialize_memory(address_t start_addr, address_t end_addr)
    {
        assert(is_aligned(start_addr));

        std::array<ChunkList, bin_count> initial_chunk_lists;
        address_t last_addr = start_addr;
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

    size_t get_bin_size(size_t payload_size) const
    {
        assert(contains_bin_with_chunk_size(payload_size));

        const bin_t &bin = get_bin_with_chunk_size(payload_size);
        return bin.chunk_list.size();
    }

    size_t get_total_chunks_count() const
    {
        size_t total_size = 0;
        for (const bin_t &bin : bins) {
            total_size += bin.chunk_list.size();
        }
        return total_size;
    }

    /**
     * Allocates a chunk with exactly payload size. If it is not possible nullptr is returned.
     * @param payload_size Chunk's payload size.
     * @return May return nullptr.
     */
    chunk_t * allocate_chunk(size_t payload_size)
    {
        assert(contains_bin_with_chunk_size(payload_size));
        
        chunk_t *free_chunk = nullptr;
        if (contains_bin_with_chunk_size(payload_size)) {
            bin_t &bin = get_bin_with_chunk_size(payload_size);
            free_chunk = allocate_in_bin(bin);
        }

        if (!free_chunk) {
            free_chunk = allocate_in_bin_with_higher_chunk_size(payload_size);
        }
        return free_chunk;
    }

    /**
     * Adds a free chunk. Preferably is a situation, when last allocation failed, so it is
     * obvious that small bins needs some free chunk.
     * @param chunk Chunk with arbitrary size. Will be moved inside small bins. May also be
     * split.
     * @return Redundant chunk that was created during splitting of given chunk.
     * May not be null.
     */
    void add_chunk(chunk_t *chunk)
    {
        assert(chunk);
        assert(contains_bin_with_chunk_size(chunk->payload_size));

        move_chunk_to_correct_bin(chunk);
    }

    void split_and_store_chunk(chunk_t *chunk)
    {
        assert(!contains_bin_with_chunk_size(chunk->payload_size));
        disperse_chunk_into_all_bins(chunk);
    }

    /// Returns bool whether given size fits in some small bin.
    bool contains_bin_with_chunk_size(size_t payload_size) const
    {
        return min_chunk_size_for_bins <= payload_size &&
                payload_size <= max_chunk_size_for_bins &&
                (payload_size % gap_between_bins == 0);
    }

    bool try_remove_chunk_from_list(chunk_t *chunk)
    {
        if (contains_bin_with_chunk_size(chunk->payload_size)) {
            bin_t &bin = get_bin_with_chunk_size(chunk->payload_size);
            bin.chunk_list.remove_chunk(chunk);
            return true;
        }
        else {
            return false;
        }
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

    chunk_t * allocate_in_bin(bin_t &bin)
    {
        chunk_t *free_chunk = bin.chunk_list.find_free_chunk();
        if (free_chunk) {
            bin.chunk_list.remove_chunk(free_chunk);
        }
        return free_chunk;
    }

    chunk_t * allocate_in_bin_with_higher_chunk_size(size_t payload_size)
    {
        chunk_t *new_chunk = nullptr;
        chunk_t *bigger_free_chunk = find_and_pop_smallest_free_chunk(payload_size);
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

    /// Finds first free chunk with size greater than payload_size parameter.
    /// If there is no such chunk, nullptr is returned.
    chunk_t * find_and_pop_smallest_free_chunk(size_t payload_size)
    {
        for (bin_t &bin : bins) {
            if (bin.chunk_sizes > payload_size) {
                chunk_t *free_chunk = bin.chunk_list.find_free_chunk();
                if (free_chunk) {
                    bin.chunk_list.remove_chunk(free_chunk);
                    return free_chunk;
                }
            }
        }
        return nullptr;
    }

    void disperse_chunk_into_all_bins(chunk_t *chunk)
    {
        assert(chunk);

        bool chunk_can_be_split = true;
        while (chunk_can_be_split) {
            for (bin_t &bin : bins) {
                if (is_chunk_splittable(chunk, bin.chunk_sizes)) {
                    chunk_t *new_chunk = split_chunk(chunk, bin.chunk_sizes);
                    bin.chunk_list.prepend_chunk(new_chunk);
                }
                else {
                    chunk_can_be_split = false;
                    break;
                }
            }
        }
        chunk_t *redundant_chunk = chunk;
        assert(contains_bin_with_chunk_size(redundant_chunk->payload_size));
        move_chunk_to_correct_bin(redundant_chunk);
    }

    void move_chunk_to_correct_bin(chunk_t *chunk)
    {
        assert(chunk);
        // If this assertion fails, try to lower min_chunk_size_for_bins member.
        assert(contains_bin_with_chunk_size(chunk->payload_size));

        bin_t &bin = get_bin_with_chunk_size(chunk->payload_size);
        bin.chunk_list.prepend_chunk(chunk);
    }

    bin_t & get_bin_with_chunk_size(size_t chunk_size)
    {
        bin_t &bin = bins[get_index_of_bin(chunk_size)];
        assert(bin.chunk_sizes == chunk_size);
        return bin;
    }

    const bin_t & get_bin_with_chunk_size(size_t chunk_size) const
    {
        const bin_t &bin = bins[get_index_of_bin(chunk_size)];
        assert(bin.chunk_sizes == chunk_size);
        return bin;
    }

    size_t get_index_of_bin(size_t chunk_size) const
    {
        return (chunk_size - min_chunk_size_for_bins) / gap_between_bins;
    }
};


#endif //SMALL_BINS_HPP
