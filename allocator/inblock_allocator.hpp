#include <array>
#include <cstddef>
#include <tuple>
#include <cassert>
#include "allocator_exception.hpp"

template <typename T, typename HeapHolder> class inblock_allocator;
template <typename T, typename HeapHolder> class UnsortedBin;
template <typename T, typename HeapHolder> class SmallBins;


struct chunk_header_t {
    chunk_header_t *prev;
    chunk_header_t *next;
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

constexpr size_t header_size = sizeof(chunk_header_t);




template <typename T, typename HeapHolder>
class SmallBins {
public:
    static constexpr size_t gap_between_bins = 8;
    static constexpr size_t bin_count = 5;
    static constexpr size_t min_chunk_size = 16;
    static constexpr size_t max_chunk_size = min_chunk_size + (bin_count * gap_between_bins);
    static constexpr size_t type_size = inblock_allocator<T, HeapHolder>::type_size;

    explicit SmallBins(inblock_allocator<T, HeapHolder> &allocator)
        : allocator{allocator}
    {
        for (size_t i = 0; i < bins.size(); ++i) {
            size_t chunk_size = min_chunk_size + i * gap_between_bins;
            bins[i] = bin_t{chunk_size, nullptr};
        }
    }

    T * alloc(size_t count)
    {
        const size_t num_bytes = size_in_bytes(count);
        if (!contains_bin_with_chunk_size(num_bytes)) {
            throw AllocatorException{"Wrong count specified"};
        }

        bin_t &bin = get_bin_with_chunk_size(num_bytes);
        chunk_t *free_chunk = find_free_chunk_in_bin(bin);
        if (free_chunk) {
            return allocator.use_chunk(free_chunk);
        }
        else {
            return allocate_in_bin_with_higher_chunk_size(num_bytes);
        }
    }


    void free(void *ptr)
    {

    }

    /// Returns bool whether given size fits in some small bin.
    bool fits_in_small_bin(size_t count)
    {
        return contains_bin_with_chunk_size(size_in_bytes(count));
    }

private:
    struct bin_t {
        size_t chunk_sizes;
        chunk_t *first_chunk;
    };
    std::array<bin_t, bin_count> bins;
    inblock_allocator<T, HeapHolder> &allocator;


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

    T * allocate_in_bin_with_higher_chunk_size(size_t num_bytes)
    {
        chunk_t *new_chunk = nullptr;
        chunk_t *bigger_free_chunk = find_smallest_free_chunk(num_bytes);
        if (bigger_free_chunk) {
            new_chunk = split(bigger_free_chunk, num_bytes);
        }
        else {
            new_chunk = allocator.get_chunk(num_bytes);
        }
        return allocator.use_chunk(new_chunk);
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
    static const void *start_addr;
    static size_t size;

    void operator()(void *ptr, size_t n_bytes) {
        start_addr = ptr;
        size = n_bytes;
    };

private:

};


template<typename T, typename HeapHolder>
class inblock_allocator {
public:
    using value_type = T;
    static constexpr size_t type_size = sizeof(T);

    inblock_allocator()
        : small_bins{*this}
    {}

    T * allocate(size_t n)
    {

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
    SmallBins<T, HeapHolder> small_bins;
    LargeBin large_bin;
    UnsortedBin<T, HeapHolder> unsorted_bin;
};
