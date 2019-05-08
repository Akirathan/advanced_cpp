#define BOOST_TEST_MODULE My_test

#include <boost/test/included/unit_test.hpp>
#include <thread>

#include "kuba_concurrent_bitmap.h"

static std::vector<uint32_t> generate_many_keys_to_same_leaf(size_t key_count)
{
    std::vector<std::bitset<32>> keys;
    keys.resize(key_count);

    // Same l0, l1, l2
    for (size_t i = 0; i < 18; ++i) {
        bool rand_bit = std::rand() % 2;
        for (std::bitset<32> &key : keys) {
            key[i] = rand_bit;
        }
    }

    // Other leaf, bit
    for (size_t i = 18; i < 32; ++i) {
        for (std::bitset<32> &key : keys) {
            key[i] = std::rand() % 2;
        }
    }

    std::vector<uint32_t> ulong_keys;
    for (const std::bitset<32> &key : keys) {
        ulong_keys.push_back(static_cast<uint32_t>(key.to_ulong()));
    }
    return ulong_keys;
}

BOOST_AUTO_TEST_CASE(many_keys_into_same_leaf_simultaneously)
{
    const size_t thread_count = 8;
    const size_t keys_per_thread = 2048;
    const size_t key_count = thread_count * keys_per_thread;

    kuba::concurrent_bitmap bitmap;
    std::vector<std::thread> threads;
    std::vector<uint32_t> keys = generate_many_keys_to_same_leaf(key_count);

    for (size_t thread_idx = 0; thread_idx < thread_count; ++thread_idx) {
        threads.emplace_back([=, &keys, &bitmap](){
            for (size_t key_idx = thread_idx * keys_per_thread;
                 key_idx < (thread_idx * keys_per_thread) + keys_per_thread;
                 ++key_idx)
            {
                bitmap.set(keys[key_idx], true);
            }
        });
    }

    for (std::thread &thread : threads) {
        thread.join();
    }

    for (uint32_t key : keys) {
        BOOST_CHECK(bitmap.get(key));
    }

    // Check that only one leaf was created.
    nodes_count node_count = bitmap.get_nodes_count();
    BOOST_CHECK(node_count.inner_nodes_count == 3);
    BOOST_CHECK(node_count.leaves_count == 1);
}
