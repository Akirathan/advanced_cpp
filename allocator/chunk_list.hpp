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
        return first_chunk == nullptr;
    }

    size_t size() const
    {
        if (is_empty()) {
            return 0;
        }

        if (contains_just_one_element()) {
            return 1;
        }

        size_t list_size = 1;
        chunk_t *chunk = first_chunk->next;
        while (chunk != first_chunk) {
            list_size++;
            chunk = chunk->next;
        }

        return list_size;
    }

    void prepend_chunk(chunk_t *chunk)
    {
        assert(chunk);
        chunk_t *last = nullptr;

        if (!first_chunk) {
            first_chunk = chunk;
            return;
        }

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

        first_chunk = chunk;
    }

    chunk_t * find_free_chunk() const
    {
        if (is_empty()) {
            return nullptr;
        }

        if (contains_just_one_element()) {
            return first_chunk;
        }
        else {
            return first_chunk->prev;
        }
    }

    chunk_t * pop_first_chunk()
    {
        if (is_empty()) {
            return nullptr;
        }

        if (contains_just_one_element()) {
            chunk_t *old_first_chunk = first_chunk;
            first_chunk = nullptr;
            return old_first_chunk;
        }

        chunk_t *old_first_chunk = first_chunk;
        if (first_chunk->next) {
            first_chunk = first_chunk->next;
            remove_chunk(old_first_chunk);
        }
        else {
            old_first_chunk = nullptr;
            first_chunk = nullptr;
        }

        return old_first_chunk;
    }

    void remove_chunk(chunk_t *chunk)
    {
        if (links_to_self(chunk)) {
            chunk->next = nullptr;
            chunk->prev = nullptr;
        }

        chunk_t *prev = chunk->prev;
        chunk_t *next = chunk->next;
        chunk->next = nullptr;
        chunk->prev = nullptr;

        if (prev && next) {
            link_chunks(prev, next);
        }

        if (first_chunk == chunk) {
            first_chunk = nullptr;
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

    bool contains_just_one_element() const
    {
        if (is_empty()) {
            return false;
        }

        if (links_to_self(first_chunk)) {
            return true;
        }

        return has_no_links(first_chunk);
    }

    static bool links_to_self(const chunk_t *chunk)
    {
        return chunk->prev == chunk && chunk->next == chunk;
    }

    bool has_next(const chunk_t *chunk) const
    {
        return chunk->next;
    }

    bool has_prev(const chunk_t *chunk) const
    {
        return chunk->prev;
    }

    bool has_no_links(const chunk_t *chunk) const
    {
        return chunk->next == nullptr && chunk->prev == nullptr;
    }

};


#endif //CHUNK_LIST_HPP
