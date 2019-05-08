// concurrent_bitmap.h
// Jakub Marousek NPRG051 2018/2019

#include <iostream>
#include <memory>
#include <vector>
#include <cstdint>
#include <atomic>
#include <mutex>
#include "test_common.hpp"

namespace kuba {

// we use a tree solution, very similar to the one described in
// the assignment

// key and value types are common for the whole source code
struct concurrent_bitmap_traits {
    using key_type = uint32_t;
    using value_type = bool;
};

// a single node in the tree hierarchy
class concurrent_bitmap_node : public concurrent_bitmap_traits {

    // array of subsequent nodes -- we use raw arrays to save space and speed up
    // things
    concurrent_bitmap_node **nodes;

    // array of data holders
    std::atomic<uint8_t> *data;

    // range of bits which the level of this node processes
    // min_bit included, max_bit excluded
    int min_bit, max_bit;

    // if this is not a leaf node, it contains subsequent nodes
    // if this is a leaf node, it contains data
    // (the other thing is always nullptr)
    bool is_leaf;

    // 0 is the root of the hierarchy
    int level;

    // locked in case we're editing the nodes (creating a new sub-node)
    std::mutex lock;

public:
    concurrent_bitmap_node(int level);

    ~concurrent_bitmap_node();

    // for the leaf node, there are two indexes we're interested in, the index
    // of the containing uint8_t and the index of the bit within uint8_t
    // for the non-leaf node, the second index is always zero
    std::pair<int, int> get_indexes(key_type key) const;

    value_type get(key_type key) const;

    void set(key_type key, value_type value);

    nodes_count get_nodes_count(nodes_count accumulator) const;
};

class concurrent_bitmap : public concurrent_bitmap_traits {
    concurrent_bitmap_node root{0};
public:
    value_type get(key_type key) const;

    void set(key_type key, value_type value);

    nodes_count get_nodes_count() const;
};

} // namespace kuba
