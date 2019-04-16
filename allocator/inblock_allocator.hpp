#include <cassert>
#include <cstddef>
#include <cstdint>
#include <boost/log/trivial.hpp>
#include "common.hpp"
#include "chunk.hpp"
#include "allocator_exception.hpp"

using chunk_list_t = chunk_t *;

class inblock_allocator_heap {
public:
    static chunk_list_t chunk_list;

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
        BOOST_LOG_TRIVIAL(info) << "Constructing allocator";
    }

    inblock_allocator(const inblock_allocator<T, HeapHolder> &) noexcept
    {
        BOOST_LOG_TRIVIAL(info) << "Copy-constructing allocator";
    }

    ~inblock_allocator() noexcept
    {
        BOOST_LOG_TRIVIAL(info) << "Destructing allocator";
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

        new_chunk->used = true;
        return reinterpret_cast<T *>(get_chunk_data(new_chunk));
    }

    void deallocate(T *ptr, size_t n) noexcept
    {
        size_t bytes_num = byte_count(n);
        bytes_num = align_size_up(bytes_num);

        BOOST_LOG_TRIVIAL(debug) << "Releasing " << bytes_num << " bytes.";

        chunk_t *freed_chunk = get_chunk_from_payload_addr(reinterpret_cast<address_t>(ptr));
        freed_chunk->used = false;

        if (!remove_from_list(freed_chunk)) {
            BOOST_LOG_TRIVIAL(warning) << "Chunk to deallocate was not found in list";
        }
    }

    const chunk_t * get_chunk_list() const
    {
        return chunk_list;
    }

private:
    const address_t heap_start_addr = heap_type::get_start_addr();
    const address_t heap_end_addr = heap_type::get_end_addr();
    const size_t heap_size = heap_type::get_size();
    chunk_list_t &chunk_list = heap_type::chunk_list;

    chunk_t * allocate_chunk(size_t payload_size)
    {
        if (!chunk_list) {
            chunk_list = initialize_chunk(heap_start_addr, payload_size);
            return chunk_list;
        }

        chunk_t *chunk = nullptr;

        chunk = try_to_allocate_before_first_chunk(payload_size);
        if (chunk) {
            return chunk;
        }

        chunk_t *last_chunk = nullptr;
        std::tie(chunk, last_chunk) = try_to_allocate_between_chunks(payload_size);
        if (chunk) {
            return chunk;
        }
        assert(last_chunk);


        chunk = try_to_allocate_after_last_chunk(payload_size, last_chunk);
        if (chunk) {
            return chunk;
        }

        return nullptr;
    }

    chunk_t *try_to_allocate_after_last_chunk(size_t payload_size, chunk_t *last_chunk)
    {
        assert(last_chunk);
        assert(!last_chunk->next);
        const size_t required_chunk_size = chunk_header_size + payload_size;

        if (get_space_after_last_chunk(last_chunk) >= required_chunk_size) {
            chunk_t *new_chunk = initialize_chunk(get_address_after_chunk(last_chunk), payload_size);

            last_chunk->next = new_chunk;

            return new_chunk;
        }
        else {
            return nullptr;
        }
    }

    /// Returns pair<allocated_chunk, last_chunk>.
    std::pair<chunk_t *, chunk_t *> try_to_allocate_between_chunks(size_t payload_size)
    {
        const size_t required_chunk_size = chunk_header_size + payload_size;

        chunk_t *last_chunk = chunk_list;
        chunk_t *chunk = last_chunk->next;
        while (chunk) {
            if (get_space_between_chunks(last_chunk, chunk) >= required_chunk_size) {
                chunk_t *new_chunk = initialize_chunk_between(last_chunk, chunk, payload_size);
                return std::make_pair(new_chunk, last_chunk);
            }
            last_chunk = chunk;
            chunk = chunk->next;
        }
        return std::make_pair(nullptr, last_chunk);
    }

    chunk_t * try_to_allocate_before_first_chunk(size_t payload_size)
    {
        const size_t required_chunk_size = chunk_header_size + payload_size;

        if (get_space_before_first_chunk() >= required_chunk_size) {
            chunk_t *new_chunk = initialize_chunk(heap_start_addr, payload_size);

            chunk_t *old_first_chunk = chunk_list;
            new_chunk->next = old_first_chunk;
            chunk_list = new_chunk;

            return new_chunk;
        }
        else {
            return nullptr;
        }
    }

    chunk_t * initialize_chunk_between(chunk_t *first_chunk, chunk_t *second_chunk, size_t payload_size)
    {
        assert(get_space_between_chunks(first_chunk, second_chunk) >= chunk_header_size + payload_size);

        chunk_t *new_chunk = initialize_chunk(get_address_after_chunk(first_chunk), payload_size);
        insert_between(first_chunk, second_chunk, new_chunk);
        return new_chunk;
    }

    bool remove_from_list(chunk_t *chunk_to_remove) noexcept
    {
        if (!chunk_list) {
            return false;
        }
        else if (!chunk_list->next) {
            if (chunk_list == chunk_to_remove) {
                chunk_list = nullptr;
                return true;
            }
            else {
                return false;
            }
        }
        else if (chunk_list == chunk_to_remove) {
            chunk_t *second_chunk = chunk_list->next;
            chunk_list->next = nullptr;
            chunk_list = second_chunk;
            return true;
        }

        chunk_t *last_chunk = chunk_list;
        chunk_t *chunk = chunk_list->next;
        while (chunk) {
            if (chunk == chunk_to_remove) {
                chunk_t *next = chunk->next;
                chunk->next = nullptr;

                if (next) {
                    last_chunk->next = next;
                }
                else {
                    last_chunk->next = nullptr;
                }
                return true;
            }
            last_chunk = chunk;
            chunk = chunk->next;
        }
        return false;
    }

    address_t get_address_after_chunk(const chunk_t *chunk) const
    {
        auto chunk_addr = reinterpret_cast<address_t>(chunk);
        return chunk_addr + get_chunk_size(chunk);
    }

    size_t get_space_before_first_chunk() const
    {
        const chunk_t *first_chunk = chunk_list;
        if (!first_chunk) {
            return heap_size;
        }
        else {
            return diff(heap_start_addr, reinterpret_cast<address_t>(first_chunk));
        }
    }

    size_t get_space_between_chunks(const chunk_t *first_chunk, const chunk_t *second_chunk) const
    {
        auto first_chunk_end = reinterpret_cast<address_t>(first_chunk) + get_chunk_size(first_chunk);
        auto second_chunk_start = reinterpret_cast<address_t>(second_chunk);
        return diff(first_chunk_end, second_chunk_start);
    }

    size_t get_space_after_last_chunk(const chunk_t *last_chunk) const
    {
        auto last_chunk_end = reinterpret_cast<address_t>(last_chunk) + get_chunk_size(last_chunk);
        return diff(last_chunk_end, heap_end_addr);
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
