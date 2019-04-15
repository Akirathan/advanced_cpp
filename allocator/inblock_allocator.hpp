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

        chunk_t *new_chunk = allocate_chunk(bytes_num);
        if (!new_chunk) {
            throw AllocatorException{"Run out of memory"};
        }

        return reinterpret_cast<T *>(get_chunk_data(new_chunk));
    }

    chunk_t * allocate_chunk(size_t payload_size)
    {
        const size_t required_chunk_size = chunk_header_size + payload_size;

        // No chunks yet.
        if (!chunk_list) {
            chunk_list = initialize_chunk(heap_start_addr, payload_size);
            return chunk_list;
        }

        // Try to allocate between heap start and first chunk.
        if (get_space_before_first_chunk(chunk_list) >= required_chunk_size) {
            chunk_t *new_chunk = initialize_chunk(heap_start_addr, payload_size);

            chunk_t *old_first_chunk = chunk_list;
            new_chunk->next = old_first_chunk;
            chunk_list = new_chunk;

            return new_chunk;
        }

        chunk_t *last_chunk = chunk_list;
        chunk_t *chunk = last_chunk->next;

        while (chunk) {
            if (get_space_between_chunks(last_chunk, chunk) >= required_chunk_size) {
                return initialize_chunk_between(last_chunk, chunk, payload_size);
            }
            last_chunk = chunk;
            chunk = chunk->next;
        }

        // Try to allocate between last chunk and heap_end.
        if (get_space_after_last_chunk(last_chunk) >= required_chunk_size) {
            chunk_t *new_chunk = initialize_chunk(get_address_after_chunk(last_chunk), payload_size);

            last_chunk->next = new_chunk;

            return new_chunk;
        }

        return nullptr;
    }

    void deallocate(T *ptr, size_t n) noexcept
    {
        size_t bytes_num = byte_count(n);
        bytes_num = align_size_up(bytes_num);

        BOOST_LOG_TRIVIAL(debug) << "Releasing " << bytes_num << " bytes.";

        chunk_t *freed_chunk = get_chunk_from_payload_addr(reinterpret_cast<address_t>(ptr));

        freed_chunk->used = false;
        freed_chunk->next = nullptr;
    }

private:
    const address_t heap_start_addr = heap_type::get_start_addr();
    const address_t heap_end_addr = heap_type::get_end_addr();
    const size_t heap_size = heap_type::get_size();
    chunk_t *chunk_list;

    chunk_t * initialize_chunk_between(chunk_t *first_chunk, chunk_t *second_chunk, size_t payload_size)
    {
        assert(get_space_between_chunks(first_chunk, second_chunk) >= chunk_header_size + payload_size);

        chunk_t *new_chunk = initialize_chunk(get_address_after_chunk(first_chunk), payload_size);
        insert_between(first_chunk, second_chunk, new_chunk);
        return new_chunk;
    }

    address_t get_address_after_chunk(const chunk_t *chunk) const
    {
        auto chunk_addr = reinterpret_cast<address_t>(chunk);
        return chunk_addr + get_chunk_size(chunk);
    }

    size_t get_space_between_chunks(const chunk_t *first_chunk, const chunk_t *second_chunk) const
    {
        // TODO
    }

    size_t get_space_after_last_chunk(const chunk_t *last_chunk) const
    {
        // TODO
    }

    void insert_between(chunk_t *first_chunk, chunk_t *second_chunk, chunk_t *chunk_to_insert) const
    {
        assert(first_chunk->next == second_chunk);

        first_chunk->next = chunk_to_insert;
        chunk_to_insert->next = second_chunk;
    }

    size_t byte_count(size_t type_count) const
    {
        return type_size * type_count;
    }

};
