#ifndef UNSORTED_BIN_HPP
#define UNSORTED_BIN_HPP

#include <array>
#include <cstddef>
#include <cassert>
#include "chunk.hpp"
#include "chunk_list.hpp"

class UnsortedBin {
public:

    void store_chunk(chunk_t *chunk)
    {
        // TODO
    }

    void store_chunks(const ChunkList &chunks)
    {
        // TODO
    }

    /**
     * Finds a chunk which is greater than payload_size and removes it from the list.
     * @param payload_size
     * @return May return nullptr if no such chunk is found.
     */
    chunk_t * get_chunk_with_size_at_least(size_t payload_size)
    {
        // TODO
        return nullptr;
    }

    /**
     * Returns first unsorted chunk.
     * @return May return nullptr if there is no chunk.
     */
    chunk_t * get_first_chunk()
    {
        return unsorted_chunks.pop_first_chunk();
    }

private:
    ChunkList unsorted_chunks;
};


#endif //UNSORTED_BIN_HPP
