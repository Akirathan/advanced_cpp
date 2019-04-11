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
        : stop_traversal{false}
    {
        initialize_memory();
    }

    bool operator==(const inblock_allocator &) const
    {
        return false;
    }

    bool operator!=(const inblock_allocator &rhs) const
    {
        return !(this == rhs);
    }

    T * allocate(size_t n)
    {
        size_t bytes_num = byte_count(n);
        bytes_num = align_size_up(bytes_num);

        if (allocation_fits_in_small_bins(bytes_num)) {
            return allocate_in_small_bins(bytes_num);
        }
        else {
            return allocate_in_large_bin(bytes_num);
        }
    }

    void deallocate(T *ptr, size_t n) noexcept
    {
        (void) n;

        chunk_t *freed_chunk = get_chunk_from_payload_addr(reinterpret_cast<intptr_t>(ptr));
        freed_chunk->used = false;
        freed_chunk->prev = nullptr;
        freed_chunk->next = nullptr;
        large_bin.store_chunk(freed_chunk);
    }

    intptr_t get_chunk_region_start_addr() const
    {
        return chunk_region_start_addr;
    }

    intptr_t get_chunk_region_end_addr() const
    {
        return chunk_region_end_addr;
    }

    const SmallBins & get_small_bins() const
    {
        return small_bins;
    }

    const LargeBin & get_large_bin() const
    {
        return large_bin;
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
    bool stop_traversal;

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

    bool allocation_fits_in_small_bins(size_t bytes_num) const
    {
        return bytes_num <= SmallBins::max_chunk_size_for_bins;
    }

    T * allocate_in_small_bins(size_t bytes_num)
    {
        chunk_t *chunk_for_allocation = small_bins.allocate_chunk(bytes_num);
        if (!chunk_for_allocation) {
            chunk_t *bigger_chunk = find_chunk_with_size_at_least(bytes_num);
            if (!bigger_chunk) {
                throw AllocatorException{"Run out of memory"};
            }

            chunk_for_allocation = try_split_and_put_residue_in_large_bin(bigger_chunk, bytes_num);
            refill_small_bins();
        }
        else {
            // During allocation in small bins, redundant chunk may be created.
            if (small_bins.contains_redundant_chunk()) {
                chunk_t *redundant_chunk = small_bins.pop_redundant_chunk();
                redundant_chunk->prev = nullptr;
                redundant_chunk->next = nullptr;
                large_bin.store_chunk(redundant_chunk);
            }
        }

        return use_chunk(chunk_for_allocation);
    }

    T * allocate_in_large_bin(size_t bytes_num)
    {
        chunk_t *large_chunk = find_chunk_with_size_at_least(bytes_num);
        if (!large_chunk) {
            throw AllocatorException{"Run out of memory"};
        }

        chunk_t *desired_chunk = try_split_and_put_residue_in_large_bin(large_chunk, bytes_num);
        return use_chunk(desired_chunk);
    }

    chunk_t * try_split_and_put_residue_in_large_bin(chunk_t *chunk, size_t desired_payload_size)
    {
        chunk_t *new_chunk = chunk;
        if (is_chunk_splittable(chunk, desired_payload_size)) {
            new_chunk = split_chunk(chunk, desired_payload_size);
            large_bin.store_chunk(chunk);
        }
        return new_chunk;
    }

    void refill_small_bins()
    {
        chunk_t *some_chunk = large_bin.pop_first_chunk();
        if (!some_chunk) {
            return;
        }

        chunk_t *redundant_chunk = small_bins.add_chunk(some_chunk);
        assert(redundant_chunk);
        large_bin.store_chunk(redundant_chunk);
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
        auto free_chunks = find_free_mem_region_with_size_at_least(payload_size);
        if (free_chunks.empty()) {
            return nullptr;
        }
        chunk_t *large_chunk = join_chunks(free_chunks);
        assert(large_chunk->payload_size >= payload_size);
        return large_chunk;
    }

    chunk_t * join_chunks(const std::vector<chunk_t *> &chunks)
    {
        chunk_t *first_chunk = chunks[0];
        remove_chunk_from_any_list(first_chunk);
        for (auto chunk_it = chunks.begin() + 1; chunk_it < chunks.end(); chunk_it++) {
            chunk_t *chunk = *chunk_it;
            remove_chunk_from_any_list(chunk);
            ::join_chunks(first_chunk, chunk);
        }
        return first_chunk;
    }

    void remove_chunk_from_any_list(chunk_t *chunk)
    {
        if (small_bins.try_remove_chunk_from_list(chunk)) {
            return;
        }

        // If chunk was not in small_bins, it has to be in large bin.
        assert(large_bin.try_remove_chunk_from_list(chunk));
    }

    /// May return vector with size 0 if not found.
    std::vector<chunk_t *> find_free_mem_region_with_size_at_least(size_t minimal_chunks_size)
    {
        std::vector<chunk_t *> neighbouring_free_chunks;
        size_t neighbouring_free_chunks_size = 0;
        bool in_free_chunks = false;
        traverse_memory([&neighbouring_free_chunks, &neighbouring_free_chunks_size, &in_free_chunks,
                                &minimal_chunks_size, this](chunk_t *chunk) {
            if (!chunk->used) {
                if (!in_free_chunks) {
                    in_free_chunks = true;
                    neighbouring_free_chunks.clear();
                    neighbouring_free_chunks.push_back(chunk);
                    neighbouring_free_chunks_size = chunk->payload_size;
                }
                else if (in_free_chunks) {
                    neighbouring_free_chunks.push_back(chunk);
                    neighbouring_free_chunks_size += chunk_header_size + chunk->payload_size;

                    if (neighbouring_free_chunks_size >= minimal_chunks_size) {
                        this->stop_traversal = true;
                    }
                }
            }
            else if (chunk->used) {
                in_free_chunks = false;
                neighbouring_free_chunks.clear();
                neighbouring_free_chunks_size = 0;
            }
        });
        return neighbouring_free_chunks;
    }

    // TODO: Implement with iterator
    void traverse_memory(std::function<void(chunk_t *)> func)
    {
        intptr_t start_addr = chunk_region_start_addr;
        intptr_t end_addr = chunk_region_end_addr;

        auto *chunk = reinterpret_cast<chunk_t *>(start_addr);
        while (start_addr != end_addr && !stop_traversal) {
            start_addr += get_chunk_size(chunk);
            func(chunk);
            chunk = next_chunk_in_mem(chunk);
        }

        stop_traversal = false;
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
