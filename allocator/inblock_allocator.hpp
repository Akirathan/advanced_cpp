#ifndef INBLOCK_ALLOCATOR_HPP_
#define INBLOCK_ALLOCATOR_HPP_

#include <array>
#include <cstddef>
#include <tuple>
#include <cassert>
#include <vector>
#include "allocator_exception.hpp"

template <typename T, typename HeapHolder> class inblock_allocator;
template <typename T, typename HeapHolder> class UnsortedBin;
template <typename T, typename HeapHolder> class SmallBins;

struct chunk_header_t {
    chunk_header_t *prev;
    chunk_header_t *next;
    /// Payload must not be zero, unless this is the last (delimiter) chunk.
    size_t payload_size;
    bool used;
};

using chunk_t = chunk_header_t;

constexpr size_t align_size(size_t size, size_t alignment) noexcept
{
    size += size % alignment;
    return size;
}

constexpr size_t alignment = 8;
constexpr size_t chunk_header_size_with_padding = align_size(sizeof(chunk_header_t), alignment);
constexpr size_t chunk_header_size = chunk_header_size_with_padding;
constexpr size_t min_payload_size = 16; // Must be aligned.
constexpr size_t min_chunk_size = chunk_header_size + min_payload_size;

inline bool is_aligned(intptr_t ptr)
{
    return ptr % alignment == 0;
}

inline size_t diff(intptr_t ptr, intptr_t intptr)
{
    return std::abs(ptr - intptr);
}

/// Returns true if the chunk is the last delimiter chunk in the memory.
inline bool is_last_chunk_in_mem(const chunk_t *chunk)
{
    return chunk->payload_size == 0;
}

inline bool fits_in_memory_region(intptr_t start_addr, size_t payload_size, intptr_t end_addr)
{
    start_addr += chunk_header_size_with_padding + payload_size;
    return start_addr <= end_addr;
}

/**
 * Saves one chunk into memory.
 * @param start_addr
 * @param payload_size Size of payload
 * @return
 */
inline chunk_t * initialize_chunk(intptr_t start_addr, size_t payload_size)
{
    assert(payload_size >= min_payload_size);
    // TODO: Assert alligned memory?

    chunk_header_t header{nullptr, nullptr, payload_size, false};

    auto mem_addr = reinterpret_cast<chunk_header_t *>(start_addr);
    *mem_addr = header;
    return mem_addr;
}

inline void * get_chunk_data(const chunk_t *chunk)
{
    assert(chunk != nullptr);
    assert(!is_last_chunk_in_mem(chunk));
    auto ptr = reinterpret_cast<intptr_t>(chunk);
    ptr += chunk_header_size_with_padding;
    return reinterpret_cast<void *>(ptr);
}

/// Gets total size of the chunk - not just size of its payload.
inline size_t get_chunk_size(const chunk_t *chunk)
{
    assert(chunk != nullptr);
    return chunk_header_size_with_padding + chunk->payload_size;
}

inline bool is_chunk_splittable(const chunk_t *chunk)
{
    size_t min_size_for_new_chunk = min_chunk_size;
    size_t size_left_for_old_chunk_payload = min_payload_size;

    return chunk->payload_size >= size_left_for_old_chunk_payload + min_size_for_new_chunk;
}

/**
 * Splits given chunk into two chunks, one of which has size num_bytes and
 * pointer to this chunk is returned.
 * @param chunk
 * @param num_bytes
 * @return Pointer to new chunk.
 */
inline chunk_t * split_chunk(chunk_t *chunk, size_t new_chunk_payload_size)
{
    assert(chunk != nullptr);
    assert(new_chunk_payload_size >= min_payload_size);
    assert(is_chunk_splittable(chunk));

    intptr_t chunk_end = reinterpret_cast<intptr_t>(chunk) + get_chunk_size(chunk);
    intptr_t new_chunk_payload = chunk_end - new_chunk_payload_size;
    intptr_t new_chunk_begin = new_chunk_payload - chunk_header_size;

    chunk_t *new_chunk = initialize_chunk(new_chunk_begin, new_chunk_payload_size);

    chunk->payload_size -= get_chunk_size(new_chunk);

    return new_chunk;
}

inline chunk_t * next_chunk_in_mem(const chunk_t *chunk)
{
    assert(chunk != nullptr);
    assert(!is_last_chunk_in_mem(chunk));
    auto ptr = reinterpret_cast<intptr_t>(chunk);
    ptr += chunk_header_size_with_padding;
    ptr += chunk->payload_size;
    return reinterpret_cast<chunk_t *>(ptr);
}

/**
 * Links two chunks. The direction matters.
 * @param first_chunk
 * @param second_chunk
 */
inline void link_chunks(chunk_t *first_chunk, chunk_t *second_chunk)
{
    assert(first_chunk != nullptr);
    assert(second_chunk != nullptr);

    first_chunk->next = second_chunk;
    second_chunk->prev = first_chunk;
}

/// Makes cyclic links
inline void link_chunks(const std::vector<chunk_t *> &chunks)
{
    if (chunks.size() <= 1) {
        return;
    }

    for (size_t i = 0; i < chunks.size() - 1; ++i) {
        chunk_t *first_chunk = chunks[i];
        chunk_t *second_chunk = chunks[i+1];
        link_chunks(first_chunk, second_chunk);
    }

    link_chunks(chunks[chunks.size() - 1], chunks[0]);
}


template <typename T, typename HeapHolder>
class SmallBins {
public:
    static constexpr size_t gap_between_bins = 8;
    static constexpr size_t bin_count = 5;
    static constexpr size_t min_chunk_size_for_bins = 16;
    static constexpr size_t max_chunk_size_for_bins = min_chunk_size_for_bins + (bin_count * gap_between_bins);
    static constexpr size_t type_size = sizeof(T);

    /**
     * Initializes some small bins with some chunks within the address range given as parameters.
     * Note that during runtime, the range of small bins may change - given address range is
     * only for initialization.
     * SmallBins is not required to fill the whole region with some chunks. There may be a pending
     * chunk.
     * @param start_addr
     * @param end_addr
     */
    explicit SmallBins()
        : redundant_chunks{nullptr}
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

        std::array<std::vector<chunk_t *>, bin_count> initial_chunks;
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
                initial_chunks[i].push_back(new_chunk);

                start_addr += get_chunk_size(new_chunk);
                last_addr = start_addr;
            }
        }

        for (const std::vector<chunk_t *> &chunks : initial_chunks) {
            link_chunks(chunks);
        }

        for (size_t i = 0; i < bins.size(); i++) {
            if (initial_chunks[i].size() > 0) {
                bins[i].first_chunk = initial_chunks[i][0];
            }
        }

        return last_addr;
    }

    /// Allocates chunk with exactly count size.
    chunk_t * allocate_chunk(size_t payload_size)
    {
        assert(contains_bin_with_chunk_size(payload_size));

        bin_t &bin = get_bin_with_chunk_size(payload_size);
        chunk_t *free_chunk = find_free_chunk_in_bin(bin);
        if (free_chunk) {
            return free_chunk;
        }
        else {
            return allocate_in_bin_with_higher_chunk_size(payload_size);
        }
    }

    // TODO: change signature?
    void free(void *ptr)
    {

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

    /**
     * Redundant chunks are chunks that were generated as a residue from some split.
     * @return
     */
    bool contains_redundant_chunks() const
    {
        return redundant_chunks != nullptr;
    }

    chunk_t * get_redundant_chunks()
    {
        assert(redundant_chunks != nullptr);
        auto redundant_chunk = redundant_chunks;
        redundant_chunks = nullptr;
        return redundant_chunk;
    }

private:
    struct bin_t {
        size_t chunk_sizes;
        chunk_t *first_chunk;
    };
    std::array<bin_t, bin_count> bins;
    chunk_t *redundant_chunks;


    void initialize_bins()
    {
        for (size_t i = 0; i < bins.size(); ++i) {
            size_t chunk_size = min_chunk_size_for_bins + i * gap_between_bins;
            bins[i] = bin_t{chunk_size, nullptr};
        }
    }

    /// If @param chunk_size is smaller than bin.chunk_size, then a free chunk from
    /// this bin is splitted.
    void alloc_in_bin(bin_t &bin, size_t chunk_size)
    {

    }

    chunk_t * allocate_in_bin_with_higher_chunk_size(size_t payload_size)
    {
        chunk_t *new_chunk = nullptr;
        chunk_t *bigger_free_chunk = find_smallest_free_chunk(payload_size);
        if (bigger_free_chunk) {
            new_chunk = split_chunk(bigger_free_chunk, payload_size);
        }
        return new_chunk;
    }

    /// Returns nullptr if there is no free chunk.
    chunk_t * find_free_chunk_in_bin(const bin_t &bin)
    {

    }

    /// Finds first free chunk with size greater than payload_size parameter.
    /// If there is no such chunk, nullptr is returned.
    chunk_t * find_smallest_free_chunk(size_t payload_size)
    {

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

class LargeBin {

};

template <typename T, typename HeapHolder>
class UnsortedBin {
public:
    /**
     * Gets chunk with given size.
     * @param num_bytes
     * @return
     */
    chunk_t * get_chunk(size_t num_bytes)
    {

    }

    bool contains_chunk_with_size_at_least(size_t num_bytes) const
    {

    }
};


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
        //HeapHolder::heap::get_start_addr();
    }

    T * allocate(size_t n)
    {
        T *allocated_space = nullptr;
        if (small_bins.contains_bin_with_chunk_size(n)) {
            allocated_space = allocate_in_small_bins(n);
            if (allocated_space) {
                return allocated_space;
            }
            // Try to take chunk from unsorted_bin.
        }

    }

    chunk_t * get_chunk(size_t num_bytes)
    {

    }

    /// Allocates the chunk for user.
    T * use_chunk(chunk_t *chunk)
    {
        assert(chunk != nullptr);
    }

private:
    const intptr_t start_addr = HeapHolder::heap.get_start_addr();
    SmallBins<T, HeapHolder> small_bins;
    LargeBin large_bin;
    UnsortedBin<T, HeapHolder> unsorted_bin;

    struct initial_memory_division_t {
        intptr_t small_bins_start;
        intptr_t small_bins_end;
        intptr_t large_bin_start;
        intptr_t large_bin_end;
    };

    T * allocate_in_small_bins(size_t n)
    {
        chunk_t *allocated_chunk = small_bins.allocate_chunk(n);
        if (!allocated_chunk) {

        }
    }

    initial_memory_division_t count_initial_memory_division() const
    {

    }

};


#endif // INBLOCK_ALLOCATOR_HPP_
