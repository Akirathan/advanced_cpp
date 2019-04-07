#include <array>
#include <cstddef>
#include <tuple>
#include "allocator_exception.hpp"

template <typename T, typename HeapHolder> class inblock_allocator;


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


/// Allocates the chunk for user.
void use_chunk(chunk_t *chunk)
{

}


template <typename T, typename HeapHolder>
class SmallBins {
public:
    static constexpr size_t gap_between_bins = 8;
    static constexpr size_t bin_count = 5;
    static constexpr size_t min_chunk_size = 16;
    static constexpr size_t max_chunk_size = min_chunk_size + (bin_count * gap_between_bins);
    static constexpr size_t type_size = inblock_allocator<T, HeapHolder>::type_size;

    SmallBins()
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

        for(auto &&bin : bins) {
            if (bin.chunk_sizes == num_bytes) {
                chunk_t *free_chunk = find_free_chunk(bin.first_chunk);
                if (free_chunk) {
                    use_chunk(free_chunk);
                }
            }
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


    bool contains_bin_with_chunk_size(size_t chunk_size) const
    {
        for (auto &&bin : bins) {
            if (bin.chunk_sizes == chunk_size) {
                return true;
            }
        }
        return false;
    }

    size_t size_in_bytes(size_t count) const
    {
        return count * type_size;
    }

    chunk_t * find_free_chunk(const bin_t *bin) const
    {

    }

};

class LargeBin {

};

class UnsortedBin {

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

    T * allocate(size_t n)
    {

    }

private:
    SmallBins<T, HeapHolder> small_bins;
    LargeBin large_bin;
    UnsortedBin unsorted_bin;
};
