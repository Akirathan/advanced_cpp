// concurrent_bitmap.cpp
// Jakub Marousek NPRG051 2018/2019

#include <iostream>
#include <memory>
#include <vector>
#include <cstdint>
#include <atomic>
#include <mutex>
#include <cassert>

#include "kuba_concurrent_bitmap.h"

namespace kuba {

concurrent_bitmap_node::concurrent_bitmap_node(int level) : level(level)
{
    is_leaf = false;
    switch (level) {
        case 0:
            min_bit = 0;
            max_bit = 6;
            break;
        case 1:
            min_bit = 6;
            max_bit = 12;
            break;
        case 2:
            min_bit = 12;
            max_bit = 18;
            break;
        case 3:
            min_bit = 18;
            max_bit = 32;
            is_leaf = true;
            break;
        default:
            assert(false);
    }

    int num_possible = 1 << (max_bit - min_bit);
    if (!is_leaf) {
        nodes = new concurrent_bitmap_node *[num_possible];
        for (int i = 0; i < num_possible; i++)
            nodes[i] = nullptr;
    } else {
        data = new std::atomic<uint8_t>[num_possible / 8];
        for (int i = 0; i < num_possible / 8; i++)
            data[i].store(0);
    }
}

concurrent_bitmap_node::~concurrent_bitmap_node()
{
    if (!is_leaf) {
        int num_possible = 1 << (max_bit - min_bit);
        for (int i = 0; i < num_possible; i++)
            if (nodes[i] != nullptr)
                delete nodes[i];
        delete[] nodes;
    } else {
        delete[] data;
    }
}

std::pair<int, int> concurrent_bitmap_node::get_indexes(key_type key) const
{
    int mask = 1 << (max_bit - min_bit);
    mask--;
    mask >>= min_bit;
    int superindex = key & mask;
    if (is_leaf)
        return std::make_pair(superindex / 8, superindex % 8);
    else
        return std::make_pair(superindex, 0);
}

concurrent_bitmap_traits::value_type concurrent_bitmap_node::get(key_type key) const
{
    int idx1, idx2;
    std::tie(idx1, idx2) = get_indexes(key);
    if (is_leaf)
        return (data[idx1] & (1 << idx2)) != 0;
    else if (nodes[idx1] == nullptr)
        return false;
    else
        return nodes[idx1]->get(key);
};

void concurrent_bitmap_node::set(key_type key, value_type value)
{
    int idx1, idx2;
    std::tie(idx1, idx2) = get_indexes(key);
    if (is_leaf) {
        std::atomic<uint8_t> *d = data + idx1;
        if (value == 1)
            *d |= (1 << idx2);
        else if (value == 0)
            *d &= ~(1 << idx2);
        else
            assert(false);
    } else {
        if (nodes[idx1] == nullptr) {
            std::lock_guard<std::mutex> guard(lock);
            if (nodes[idx1] == nullptr)
                nodes[idx1] = new concurrent_bitmap_node(level + 1);
        }
        nodes[idx1]->set(key, value);
    }
}

nodes_count concurrent_bitmap_node::get_nodes_count(nodes_count accumulator) const
{
    if (is_leaf) {
        accumulator.leaves_count++;
        return accumulator;
    }
    else {
        accumulator.inner_nodes_count++;
        int array_size = 1 << (max_bit - min_bit);
        for (int i = 0; i < array_size; ++i) {
            if (nodes[i] != nullptr) {
                accumulator = nodes[i]->get_nodes_count(accumulator);
            }
        }
        return accumulator;
    }
}

concurrent_bitmap_traits::value_type concurrent_bitmap::get(key_type key) const
{
    return root.get(key);
}

void concurrent_bitmap::set(key_type key, value_type value)
{
    root.set(key, value);
}

nodes_count concurrent_bitmap::get_nodes_count() const
{
    return root.get_nodes_count(nodes_count{});
}

} // namespace kuba
