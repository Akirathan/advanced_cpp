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
struct chunk_t;

struct chunk_header_t {
    chunk_t *prev;
    chunk_t *next;
    size_t size;
    bool used;
};

struct chunk_footer_t {
    size_t size;
};

struct chunk_t {
    chunk_header_t header;
    void *data;
    chunk_footer_t footer;
};

constexpr size_t align_size(size_t size, size_t alignment) noexcept
{
    size += size % alignment;
    return size;
}

constexpr size_t alignment = 8;
constexpr size_t chunk_header_size = sizeof(chunk_header_t);
constexpr size_t chunk_footer_size = sizeof(chunk_footer_t);
constexpr size_t min_payload_size = 16;
constexpr size_t min_chunk_size = align_size(chunk_header_size + chunk_footer_size + min_payload_size, alignment);

inline bool is_aligned(intptr_t ptr)
{
    return ptr % alignment == 0;
}

inline size_t diff(intptr_t ptr, intptr_t intptr)
{
    return std::abs(ptr - intptr);
}

inline chunk_t * initialize_chunk(intptr_t start_addr, size_t size)
{
    assert(size >= min_chunk_size);
    assert(is_aligned(start_addr));
    chunk_header_t header{nullptr, nullptr, size, false};
    chunk_footer_t footer{size};
    chunk_t chunk{header, nullptr, footer};

    chunk_t *mem_addr = reinterpret_cast<chunk_t *>(start_addr);
    *mem_addr = chunk;
    return mem_addr;
}

inline void link_chunks(const std::vector<chunk_t *> &chunks)
{

}

inline void link_chunks(chunk_t *first_chunk, chunk_t *second_chunk)
{
    assert(first_chunk != nullptr);
    assert(second_chunk != nullptr);

    first_chunk->header.next = second_chunk;
    second_chunk->header.prev = first_chunk;
}


template <typename T, typename HeapHolder>
class SmallBins {
public:
    static constexpr size_t gap_between_bins = 8;
    static constexpr size_t bin_count = 5;
    static constexpr size_t min_chunk_size_for_bins = 16;
    static constexpr size_t max_chunk_size_for_bins = min_chunk_size_for_bins + (bin_count * gap_between_bins);
    static constexpr size_t type_size = inblock_allocator<T, HeapHolder>::type_size;

    /**
     * Initializes some small bins with some chunks within the address range given as parameters.
     * Note that during runtime, the range of small bins may change - given address range is
     * only for initialization.
     * @param start_addr
     * @param end_addr
     */
    explicit SmallBins(intptr_t start_addr, intptr_t end_addr)
    {
        assert(is_aligned(start_addr));
        assert(is_aligned(end_addr));
        initialize_bins();
        initialize_chunks_for_all_bins(start_addr, end_addr);
    }

    /// Allocates chunk with exactly count size.
    chunk_t * allocate_chunk(size_t count)
    {
        const size_t num_bytes = size_in_bytes(count);
        if (!contains_bin_with_chunk_size(num_bytes)) {
            throw AllocatorException{"Wrong count specified"};
        }

        bin_t &bin = get_bin_with_chunk_size(num_bytes);
        chunk_t *free_chunk = find_free_chunk_in_bin(bin);
        if (free_chunk) {
            return free_chunk;
        }
        else {
            return allocate_in_bin_with_higher_chunk_size(num_bytes);
        }
    }

    // TODO: change signature?
    void free(void *ptr)
    {

    }

    /// Returns bool whether given size fits in some small bin.
    bool fits_in_small_bin(size_t count) const
    {
        return contains_bin_with_chunk_size(size_in_bytes(count));
    }

private:
    struct bin_t {
        size_t chunk_sizes;
        chunk_t *first_chunk;
    };
    std::array<bin_t, bin_count> bins;


    void initialize_bins()
    {
        for (size_t i = 0; i < bins.size(); ++i) {
            size_t chunk_size = min_chunk_size_for_bins + i * gap_between_bins;
            bins[i] = bin_t{chunk_size, nullptr};
        }
    }

    void initialize_chunks_for_all_bins(intptr_t start_addr, intptr_t end_addr)
    {
        size_t available_size = diff(start_addr, end_addr);
        auto bin_sizes = count_memory_region_sizes_for_bins(available_size);

        for (size_t i = 0; i < bins.size(); i++) {
            initialize_bin_with_chunks(bins[i], start_addr, start_addr + bin_sizes[i]);
            start_addr += bin_sizes[i];
        }
    }

    std::array<size_t, bin_count> count_memory_region_sizes_for_bins(size_t available_size) const
    {
        std::array<size_t, bin_count> bin_sizes = {};
        for (size_t &size_for_bin : bin_sizes) {
            size_for_bin = available_size / bins.size();
        }
        // Add rest of size to first bin.
        bin_sizes[0] += available_size % bins.size();
        return bin_sizes;
    }

    void initialize_bin_with_chunks(bin_t &bin, intptr_t start_addr, intptr_t end_addr)
    {
        assert(is_aligned(start_addr));
        assert(is_aligned(end_addr));
        assert(diff(start_addr, end_addr) % bin.chunk_sizes == 0);

        std::vector<chunk_t *> chunks;

        while (start_addr < end_addr) {
            size_t chunk_size = bin.chunk_sizes;
            chunk_t *new_chunk = initialize_chunk(start_addr, chunk_size);
            chunks.push_back(new_chunk);
            start_addr += chunk_size;
        }

        link_chunks(chunks);
        link_chunks(bin.first_chunk, chunks[0]);
    }

    bool contains_bin_with_chunk_size(size_t chunk_size) const
    {
        for (auto &&bin : bins) {
            if (bin.chunk_sizes == chunk_size) {
                return true;
            }
        }
        return false;
    }

    /// If @param chunk_size is smaller than bin.chunk_size, then a free chunk from
    /// this bin is splitted.
    void alloc_in_bin(bin_t &bin, size_t chunk_size)
    {

    }

    chunk_t * allocate_in_bin_with_higher_chunk_size(size_t num_bytes)
    {
        chunk_t *new_chunk = nullptr;
        chunk_t *bigger_free_chunk = find_smallest_free_chunk(num_bytes);
        if (bigger_free_chunk) {
            new_chunk = split(bigger_free_chunk, num_bytes);
        }
        return new_chunk;
    }

    size_t size_in_bytes(size_t count) const
    {
        return count * type_size;
    }

    chunk_t * find_free_chunk_in_bin(const bin_t &bin)
    {

    }

    /// Finds first free chunk with size greater than @param num_bytes.
    /// If there is no such chunk, nullptr is returned.
    chunk_t * find_smallest_free_chunk(size_t num_bytes)
    {

    }

    /**
     * Splits given chunk into two chunks, one of which has size num_bytes and
     * pointer to this chunk is returned.
     * The other chunk is either inserted in some small bin, or given back to allocator.
     * @param chunk
     * @param num_bytes
     * @return
     */
    chunk_t * split(chunk_t *chunk, size_t num_bytes)
    {
        assert(chunk != nullptr);
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
        : small_bins{count_initial_memory_division().small_bins_start, count_initial_memory_division().small_bins_end}
    {
        initialize_memory();
        //HeapHolder::heap::get_start_addr();
    }

    T * allocate(size_t n)
    {
        T *allocated_space = nullptr;
        if (small_bins.fits_in_small_bin(n)) {
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
    }
};


#endif // INBLOCK_ALLOCATOR_HPP_
