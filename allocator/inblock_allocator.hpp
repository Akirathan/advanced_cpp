#include <cassert>
#include <cstddef>
#include <cstdint>
#include <boost/log/trivial.hpp>
#include "common.hpp"
#include "chunk.hpp"
#include "allocator_exception.hpp"


class inblock_allocator_heap {
public:
    static address_t get_start_addr()
    {
        return start_addr;
    }

    static address_t get_end_addr()
    {
        return end_addr;
    }

    static size_t get_size()
    {
        return size;
    }

    static size_t get_allocators_count()
    {
        return allocators_count;
    }

    static void increase_allocators_count()
    {
        allocators_count++;
    }

    static void decrease_allocators_count()
    {
        allocators_count--;
    }

    void operator()(void *ptr, size_t n_bytes)
    {
        if (n_bytes < min_chunk_size) {
            throw AllocatorException{"More memory needed."};
        }

        auto intptr = reinterpret_cast<address_t>(ptr);
        start_addr = align_addr(intptr, upward);
        end_addr = align_addr(start_addr + n_bytes, downward);
        size = diff(end_addr, start_addr);
    }

private:
    static address_t start_addr;
    static address_t end_addr;
    static size_t size;
    static size_t allocators_count;

    enum Direction {
        downward,
        upward
    };

    address_t align_addr(address_t ptr, Direction direction) const
    {
        if (is_aligned(ptr)) {
            return ptr;
        }
        else {
            return find_first_aligned(ptr, direction);
        }
    }

    address_t find_first_aligned(address_t ptr, Direction direction) const
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
    using heap_type = decltype(HeapHolder::heap);
    static constexpr size_t type_size = sizeof(T);

    inblock_allocator() noexcept
    {
        BOOST_LOG_TRIVIAL(debug) << "Constructing allocator";
    }

    inblock_allocator(const inblock_allocator<T, HeapHolder> &) noexcept
    {
        BOOST_LOG_TRIVIAL(info) << "Copy-constructing allocator";
    }

    ~inblock_allocator() noexcept
    {
        BOOST_LOG_TRIVIAL(debug) << "Destructing allocator";
    }

    bool operator==(const inblock_allocator &) const
    {
        return false;
    }

    bool operator!=(const inblock_allocator &) const
    {
        return true;
    }

    T * allocate(size_t n)
    {
        size_t bytes_num = byte_count(n);
        bytes_num = align_size_up(bytes_num);

        BOOST_LOG_TRIVIAL(debug) << "Allocating " << bytes_num << " bytes.";
    }

    void deallocate(T *ptr, size_t n) noexcept
    {
        size_t bytes_num = byte_count(n);
        bytes_num = align_size_up(bytes_num);

        BOOST_LOG_TRIVIAL(debug) << "Releasing " << bytes_num << " bytes.";

        chunk_t *freed_chunk = get_chunk_from_payload_addr(reinterpret_cast<address_t>(ptr));

        freed_chunk->used = false;
        freed_chunk->prev = nullptr;
        freed_chunk->next = nullptr;
    }

private:
    const address_t heap_start_addr = heap_type::get_start_addr();
    const address_t heap_end_addr = heap_type::get_end_addr();
    const size_t heap_size = heap_type::get_size();

    size_t byte_count(size_t type_count) const
    {
        return type_size * type_count;
    }
};
