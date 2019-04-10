#ifndef INBLOCK_ALLOCATOR_HPP_
#define INBLOCK_ALLOCATOR_HPP_

#include <array>
#include <cstddef>
#include <tuple>
#include <cassert>
#include <vector>
#include <cmath>
#include "allocator_exception.hpp"
#include "small_bins.hpp"
#include "large_bin.hpp"
#include "unsorted_bin.hpp"
#include "chunk_list.hpp"
#include "common.hpp"
#include "chunk.hpp"


class inblock_allocator_heap {
public:
    static intptr_t get_start_addr()
    {
        return start_addr;
    }

    static intptr_t get_end_addr()
    {
        return end_addr;
    }

    static size_t get_size()
    {
        return size;
    }

    void operator()(void *ptr, size_t n_bytes)
    {
        if (n_bytes < min_chunk_size) {
            throw AllocatorException{"More memory needed."};
        }

        auto intptr = reinterpret_cast<intptr_t>(ptr);
        start_addr = align_addr(intptr, upward);
        end_addr = align_addr(start_addr + n_bytes, downward);
        size = diff(end_addr, start_addr);
    }

private:
    static intptr_t start_addr;
    static intptr_t end_addr;
    static size_t size;

    enum Direction {
        downward,
        upward
    };

    intptr_t align_addr(intptr_t ptr, Direction direction) const
    {
        if (is_aligned(ptr)) {
            return ptr;
        }
        else {
            return find_first_aligned(ptr, direction);
        }
    }

    intptr_t find_first_aligned(intptr_t ptr, Direction direction) const
    {
        while (!is_aligned(ptr)) {
            if (direction == downward) {
                ptr--;
            }
            else if (direction == upward) {
                ptr++;
            }
        }
        return ptr;
    }

};


template<typename T, typename HeapHolder>
class inblock_allocator {
public:
    using value_type = T;
    static constexpr size_t type_size = sizeof(T);

    inblock_allocator()
    {
        initialize_memory();
    }

    T * allocate(size_t n)
    {
        if (small_bins.contains_bin_with_chunk_size(n)) {
            return allocate_in_small_bins(n);
        }
        else {
            return allocate_in_large_bin(n);
        }
    }

    intptr_t get_chunk_region_start_addr() const
    {
        return chunk_region_start_addr;
    }

    intptr_t get_chunk_region_end_addr() const
    {
        return chunk_region_end_addr;
    }

private:
    /// Denotes total size of memory that will be passed to small bins in initialization.
    /// Note that there has to be some space left in the rest of the memory at least for
    /// one minimal chunk.
    static constexpr float mem_size_for_small_bins_ratio = 0.4;
    const intptr_t heap_start_addr = HeapHolder::heap.get_start_addr();
    const intptr_t heap_end_addr = HeapHolder::heap.get_end_addr();
    const intptr_t heap_size = HeapHolder::heap.get_size();
    /// Chunk region is memory region covered by chunks and therefore used.
    /// There may be small amount of memory that is not covered by chunks.
    intptr_t chunk_region_start_addr;
    intptr_t chunk_region_end_addr;
    SmallBins small_bins;
    LargeBin large_bin;

    T * allocate_in_small_bins(size_t n)
    {
        size_t bytes_num = byte_count(n);
        chunk_t *chunk_for_allocation = small_bins.allocate_chunk(bytes_num);
        if (!chunk_for_allocation) {
            chunk_t *bigger_chunk = find_chunk_with_size_at_least(bytes_num);
            if (!bigger_chunk) {
                throw AllocatorException{"Run out of memory"};
            }

            if (is_chunk_splittable(bigger_chunk, bytes_num)) {
                chunk_for_allocation = split_chunk(bigger_chunk, bytes_num);
                large_bin.store_chunk(bigger_chunk);
            }
            else {
                chunk_for_allocation = bigger_chunk;
            }

            refill_small_bins();
        }

        return use_chunk(chunk_for_allocation);
    }

    void refill_small_bins()
    {
        //TODO: Perhaps some smarter algorithm?
        chunk_t *some_chunk = large_bin.pop_first_chunk();
        if (!some_chunk) {
            return;
        }

        chunk_t *redundant_chunk = small_bins.add_chunk(some_chunk);
        assert(redundant_chunk);
        large_bin.store_chunk(redundant_chunk);
    }

    T * allocate_in_large_bin(size_t n)
    {

    }

    /// May return nullptr if there is no chunk with size at least payload_size.
    chunk_t * find_chunk_with_size_at_least(size_t payload_size)
    {
        chunk_t *chunk = large_bin.pop_chunk_with_size_at_least(payload_size);
        if (!chunk) {
            chunk = consolidate_chunk_with_size_at_least(payload_size);
        }

        if (chunk) {
            assert(chunk->payload_size >= payload_size);
        }

        return chunk;
    }

    void initialize_memory()
    {
        chunk_region_start_addr = heap_start_addr;

        const intptr_t small_bins_start = heap_start_addr;
        const intptr_t small_bins_end = small_bins_start + std::floor(mem_size_for_small_bins_ratio * heap_size);

        const intptr_t small_bins_real_end = small_bins.initialize_memory(small_bins_start, small_bins_end);
        const intptr_t large_bin_real_end = large_bin.initialize_memory(small_bins_real_end, heap_end_addr);

        chunk_t *last_chunk = initialize_last_chunk_in_mem(large_bin_real_end);
        if (last_chunk) {
            chunk_region_end_addr = heap_end_addr;
        }
        else {
            chunk_region_end_addr = large_bin_real_end;
        }
        large_bin.store_chunk(last_chunk);
    }

    chunk_t * initialize_last_chunk_in_mem(intptr_t chunk_start_addr)
    {
        chunk_t *last_chunk = nullptr;
        if (contains_enough_space_for_chunk(chunk_start_addr, heap_end_addr)) {
            last_chunk = initialize_chunk_in_region(chunk_start_addr, heap_end_addr);
        }
        return last_chunk;
    }

    /**
     * Traverse the whole memory and searches for neighbouring free chunks which are
     * merged together. These free chunks are removed from any lists.
     * Note that this method should be called only in case when there is not enough
     * free chunks in large bins and unsorted bin.
     * @param payload_size
     * @return
     */
    chunk_t * consolidate_chunk_with_size_at_least(size_t payload_size)
    {
        // TODO
        return nullptr;
    }

    T * use_chunk(chunk_t *chunk)
    {
        assert(chunk);
        chunk->used = true;
        return reinterpret_cast<T *>(get_chunk_data(chunk));
    }

    bool contains_enough_space_for_chunk(intptr_t start_addr, intptr_t end_addr) const
    {
        return fits_in_memory_region(start_addr, min_payload_size, end_addr);
    }

    size_t byte_count(size_t type_count) const
    {
        return type_size * type_count;
    }

};


#endif // INBLOCK_ALLOCATOR_HPP_
