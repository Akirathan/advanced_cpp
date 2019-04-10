#ifndef CHUNK_LIST_HPP
#define CHUNK_LIST_HPP

#include <array>
#include <cstddef>
#include <tuple>
#include <cassert>
#include <vector>
#include "chunk.hpp"

/// Represents double-linked list of chunks
class ChunkList {
public:
    explicit ChunkList(chunk_t *chunk)
        : first_chunk{chunk}
    {}

    ChunkList()
        : ChunkList{nullptr}
    {}

    chunk_t * get_first_chunk() const
    {
        return first_chunk;
    }

    bool is_empty() const
    {
        return first_chunk != nullptr;
    }

    void prepend_chunk(chunk_t *chunk)
    {
        assert(chunk);
        chunk_t *last = nullptr;

        if (first_chunk) {
            last = first_chunk->prev;
        }

        if (first_chunk) {
            link_chunks(chunk, first_chunk);
            if (!last) {
                link_chunks(first_chunk, chunk);
            }
        }
        if (last) {
            link_chunks(last, chunk);
        }
    }

    chunk_t * find_free_chunk() const
    {
        chunk_t *chunk = first_chunk;
        chunk_t *last_chunk = chunk;
        while (chunk) {
            last_chunk = chunk;
            chunk = chunk->next;
        }
        return last_chunk;
    }

    chunk_t * pop_first_chunk()
    {
        if (!first_chunk) {
            return nullptr;
        }

        chunk_t *old_first_chunk = first_chunk;
        if (first_chunk->next) {
            first_chunk = first_chunk->next;
            remove_chunk_from_list(old_first_chunk);
        }
        else {
            old_first_chunk = nullptr;
            first_chunk = nullptr;
        }

        return old_first_chunk;
    }

    static void remove_chunk_from_list(chunk_t *chunk)
    {
        assert(chunk);

        chunk_t *prev = chunk->prev;
        chunk_t *next = chunk->next;
        chunk->next = nullptr;
        chunk->prev = nullptr;

        // Assert chunk in double-linked list.
        assert((prev && next) || (!prev && !next));

        if (prev && next) {
            link_chunks(prev, next);
        }
    }

    /**
     * Links two chunks. The direction matters.
     * @param first_chunk
     * @param second_chunk
     */
    static void link_chunks(chunk_t *first_chunk, chunk_t *second_chunk)
    {
        assert(first_chunk != nullptr);
        assert(second_chunk != nullptr);

        first_chunk->next = second_chunk;
        second_chunk->prev = first_chunk;
    }

    /// Makes cyclic links
    /// TODO: Return ChunkList?
    static void link_chunks(const std::vector<chunk_t *> &chunks)
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

private:
    chunk_t *first_chunk;
};


#endif //CHUNK_LIST_HPP