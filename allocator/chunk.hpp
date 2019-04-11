#ifndef CHUNK_HPP
#define CHUNK_HPP

#include "common.hpp"

struct chunk_header_t {
    chunk_header_t *prev;
    chunk_header_t *next;
    size_t payload_size;
    bool used;
};

using chunk_t = chunk_header_t;

constexpr size_t chunk_header_size_with_padding = align_size_up(sizeof(chunk_header_t));
constexpr size_t chunk_header_size = chunk_header_size_with_padding;
constexpr size_t min_payload_size = 8;
constexpr size_t min_chunk_size = chunk_header_size + min_payload_size;


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

/**
 * Fills given memory region with one chunk.
 * @param start_addr
 * @param end_addr
 * @return
 */
inline chunk_t * initialize_chunk_in_region(intptr_t start_addr, intptr_t end_addr)
{
    size_t space_for_chunk = diff(start_addr, end_addr);
    assert(space_for_chunk >= min_chunk_size);

    size_t payload_size = space_for_chunk - chunk_header_size;
    return initialize_chunk(start_addr, payload_size);
}

inline void * get_chunk_data(const chunk_t *chunk)
{
    assert(chunk != nullptr);
    auto ptr = reinterpret_cast<intptr_t>(chunk);
    ptr += chunk_header_size_with_padding;
    return reinterpret_cast<void *>(ptr);
}

inline chunk_t * get_chunk_from_payload_addr(intptr_t payload_addr)
{
    payload_addr -= chunk_header_size;
    return reinterpret_cast<chunk_t *>(payload_addr);
}

/// Gets total size of the chunk - not just size of its payload.
inline size_t get_chunk_size(const chunk_t *chunk)
{
    assert(chunk != nullptr);
    return chunk_header_size_with_padding + chunk->payload_size;
}

inline chunk_t * next_chunk_in_mem(const chunk_t *chunk)
{
    assert(chunk != nullptr);
    auto ptr = reinterpret_cast<intptr_t>(chunk);
    ptr += chunk_header_size_with_padding;
    ptr += chunk->payload_size;
    return reinterpret_cast<chunk_t *>(ptr);
}

inline bool is_chunk_splittable(const chunk_t *chunk, size_t new_payload_size)
{
    assert(chunk);

    size_t size_for_new_chunk = chunk_header_size + new_payload_size;
    size_t size_left_for_old_chunk_payload = min_payload_size;

    return chunk->payload_size >= size_left_for_old_chunk_payload + size_for_new_chunk;
}

/**
 * Splits given chunk into two chunks, one of which has size num_bytes and
 * pointer to this chunk is returned.
 * @param chunk
 * @param num_bytes
 * @return Pointer to new chunk.
 * TODO: Return pair of [new_chunk, old_chunk]?
 *      Abych nezapominal, ze ten old_chunk musim taky nekam dat.
 */
inline chunk_t * split_chunk(chunk_t *chunk, size_t new_chunk_payload_size)
{
    assert(chunk != nullptr);
    assert(new_chunk_payload_size >= min_payload_size);
    assert(is_chunk_splittable(chunk, new_chunk_payload_size));

    intptr_t chunk_end = reinterpret_cast<intptr_t>(chunk) + get_chunk_size(chunk);
    intptr_t new_chunk_payload = chunk_end - new_chunk_payload_size;
    intptr_t new_chunk_begin = new_chunk_payload - chunk_header_size;

    chunk_t *new_chunk = initialize_chunk(new_chunk_begin, new_chunk_payload_size);

    chunk->payload_size -= get_chunk_size(new_chunk);

    return new_chunk;
}

/**
 * Second chunk should be unlinked from any chunk list, because it will be destroyed.
 * @param first_chunk
 * @param second_chunk
 */
inline void join_chunks(chunk_t *first_chunk, chunk_t *second_chunk)
{
    assert(first_chunk);
    assert(second_chunk);
    assert(next_chunk_in_mem(first_chunk) == second_chunk);
    assert(!first_chunk->used);
    assert(!second_chunk->used);

    first_chunk->payload_size += get_chunk_size(second_chunk);
    // TODO: This is not necessary
    second_chunk->next = nullptr;
    second_chunk->prev = nullptr;
}


#endif //CHUNK_HPP
