#ifndef CHUNK_HPP
#define CHUNK_HPP

#include "common.hpp"

struct chunk_t {
    chunk_t *next;
    size_t payload_size;
    bool used;
};

constexpr size_t chunk_header_size_with_padding = align_size_up(sizeof(chunk_t));
constexpr size_t chunk_header_size = chunk_header_size_with_padding;
constexpr size_t min_payload_size = 8;
constexpr size_t min_chunk_size = chunk_header_size + min_payload_size;


inline bool fits_in_memory_region(address_t start_addr, size_t payload_size, address_t end_addr)
{
    start_addr += chunk_header_size + payload_size;
    return start_addr <= end_addr;
}

/**
 * Saves one chunk into memory.
 * @param start_addr
 * @param payload_size Size of payload
 * @return
 */
inline chunk_t * initialize_chunk(address_t start_addr, size_t payload_size)
{
    assert(payload_size >= min_payload_size);

    chunk_t header{nullptr, payload_size, false};

    auto mem_addr = reinterpret_cast<chunk_t *>(start_addr);
    *mem_addr = header;
    return mem_addr;
}

/**
 * Fills given memory region with one chunk.
 * @param start_addr
 * @param end_addr
 * @return
 */
inline chunk_t * initialize_chunk_in_region(address_t start_addr, address_t end_addr)
{
    size_t space_for_chunk = diff(start_addr, end_addr);
    assert(space_for_chunk >= min_chunk_size);

    size_t payload_size = space_for_chunk - chunk_header_size;
    return initialize_chunk(start_addr, payload_size);
}

inline void * get_chunk_data(const chunk_t *chunk)
{
    assert(chunk != nullptr);
    auto ptr = reinterpret_cast<address_t>(chunk);
    ptr += chunk_header_size_with_padding;
    return reinterpret_cast<void *>(ptr);
}

inline chunk_t * get_chunk_from_payload_addr(address_t payload_addr)
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


#endif //CHUNK_HPP
