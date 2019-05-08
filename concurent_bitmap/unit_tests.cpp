#define BOOST_TEST_MODULE My_test

#include <boost/test/included/unit_test.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <cstdlib>
#include <thread>
#include <vector>
#include <bitset>
#include "concurrent_bitmap.h"

static void setup_logging()
{
    boost::log::core::get()->set_filter
    (
        boost::log::trivial::severity >= boost::log::trivial::info
    );
}

static bool should_be_in_same_leaf(uint32_t key1, uint32_t key2)
{
    constexpr uint32_t leaf_mask = 0x0003FFFF;

    uint32_t leaf_idx_1 = key1 & leaf_mask;
    uint32_t leaf_idx_2 = key2 & leaf_mask;
    return leaf_idx_1 == leaf_idx_2;
}

static std::pair<uint32_t, uint32_t> generate_two_keys_to_same_leaf()
{
    std::bitset<32> key1;
    std::bitset<32> key2;

    // Same l0, l1, l2
    for (size_t i = 0; i < 18; ++i) {
        bool rand_bit = std::rand() % 2;
        key1[i] = rand_bit;
        key2[i] = rand_bit;
    }

    // Other leaf, bit
    for (size_t i = 18; i < 32; ++i) {
        key1[i] = std::rand() % 2;
        key2[i] = std::rand() % 2;
    }

    return std::make_pair(
        static_cast<uint32_t>(key1.to_ulong()),
        static_cast<uint32_t>(key2.to_ulong())
    );
}

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

BOOST_AUTO_TEST_SUITE(one_thread)

BOOST_AUTO_TEST_CASE(simple_test)
{
    setup_logging();
    concurrent_bitmap bitmap;
    uint32_t key = 0;
    bitmap.set(key, true);
    BOOST_TEST(bitmap.get(key) == true);
}

BOOST_AUTO_TEST_CASE(double_get)
{
    setup_logging();
    concurrent_bitmap bitmap;
    uint32_t key = 6;
    bitmap.set(key, true);
    BOOST_TEST(bitmap.get(key) == true);
    BOOST_TEST(bitmap.get(key) == true);
}

BOOST_AUTO_TEST_CASE(get_fails)
{
    setup_logging();
    concurrent_bitmap bitmap;
    bitmap.set(542, true);
    BOOST_TEST(bitmap.get(14) == false);
}

BOOST_AUTO_TEST_CASE(more_sets)
{
    setup_logging();
    concurrent_bitmap bitmap;
    for (uint32_t key = 40; key < 500; key += 7) {
        bitmap.set(key, true);
    }

    for (uint32_t key = 40; key < 500; key += 7) {
        BOOST_TEST(bitmap.get(key) == true);
    }
}

BOOST_AUTO_TEST_CASE(set_true_and_then_set_false)
{
    setup_logging();
    concurrent_bitmap bitmap;
    const uint32_t key = 42;
    bitmap.set(key, true);
    bitmap.set(key, false);
    BOOST_CHECK_EQUAL(bitmap.get(key), false);
}

BOOST_AUTO_TEST_CASE(should_be_in_same_leaf_simple_test)
{
    uint32_t addr1 = 0x000694D4;
    uint32_t addr2 = 0x000A94D4;
    BOOST_CHECK(should_be_in_same_leaf(addr1, addr2));
}

BOOST_AUTO_TEST_CASE(generate_keys_in_same_leaf_simple_test)
{
    for (int i = 0; i < 42; ++i) {
        auto keys = generate_two_keys_to_same_leaf();
        BOOST_CHECK(should_be_in_same_leaf(keys.first, keys.second));
    }
}

BOOST_AUTO_TEST_CASE(generate_many_keys_to_same_leaf_simple_test)
{
    const size_t key_count = 10;
    auto keys = generate_many_keys_to_same_leaf(key_count);
    for (size_t i = 0; i < key_count; ++i) {
        for (size_t j = i; j < key_count; ++j) {
            BOOST_CHECK(should_be_in_same_leaf(keys[i], keys[j]));
        }
    }
}

BOOST_AUTO_TEST_CASE(two_sets_in_one_leaf)
{
    concurrent_bitmap bitmap;
    uint32_t addr1 = 0x000694D4;
    uint32_t addr2 = 0x000A94D4;
    bitmap.set(addr1, true);
    bitmap.set(addr2, true);
    auto nodes_count = bitmap.get_nodes_count();
    BOOST_CHECK(nodes_count.inner_nodes_count == 3);
    BOOST_CHECK(nodes_count.leaves_count == 1);
    BOOST_CHECK(bitmap.get_bytes_count() == 2);
}

BOOST_AUTO_TEST_CASE(more_sets_in_same_leaf)
{
    setup_logging();
    concurrent_bitmap bitmap;
    nodes_count node_count;
    for (int i = 0; i < 42; ++i) {
        auto keys = generate_two_keys_to_same_leaf();
        bitmap.set(keys.first, true);
        bitmap.set(keys.second, true);
        auto new_node_count = bitmap.get_nodes_count();

        BOOST_CHECK(new_node_count.leaves_count == node_count.leaves_count ||
                    new_node_count.leaves_count == (node_count.leaves_count + 1));

        node_count = new_node_count;
    }
}

BOOST_AUTO_TEST_CASE(many_sets_in_one_leaf)
{
    setup_logging();
    concurrent_bitmap bitmap;
    nodes_count node_count{};
    const size_t key_count = 10;

    for (size_t i = 0; i < 2; ++i) {
        std::vector<uint32_t> keys = generate_many_keys_to_same_leaf(key_count);
        for (size_t key_idx = 0; key_idx < keys.size(); ++key_idx) {
            nodes_count node_count_before = bitmap.get_nodes_count();
            bitmap.set(keys[key_idx], true);
            nodes_count node_count_after = bitmap.get_nodes_count();
            if (key_idx > 0)
                BOOST_CHECK(node_count_before.leaves_count == node_count_after.leaves_count);
        }
        nodes_count new_node_count = bitmap.get_nodes_count();

        BOOST_CHECK(new_node_count.leaves_count == node_count.leaves_count ||
                    new_node_count.leaves_count == (node_count.leaves_count + 1));

        node_count = new_node_count;
    }
}


BOOST_AUTO_TEST_SUITE_END() // one_thread
BOOST_AUTO_TEST_SUITE(more_threads)


BOOST_AUTO_TEST_CASE(simple_more_thread)
{
    setup_logging();
    concurrent_bitmap bitmap;
    std::thread th1{[&bitmap](){
        bitmap.set(5, true);
    }};

    std::thread th2{[&bitmap](){
        bitmap.set(6, true);
    }};

    std::thread th3{[&bitmap](){
        bitmap.set(7, true);
    }};

    th1.join();
    th2.join();
    th3.join();

    BOOST_TEST(bitmap.get(5) == true);
    BOOST_TEST(bitmap.get(6) == true);
    BOOST_TEST(bitmap.get(7) == true);
}

BOOST_AUTO_TEST_CASE(many_threads_setting_ascending_keys)
{
    const size_t thread_num = 77;
    std::vector<std::thread> threads;
    setup_logging();
    concurrent_bitmap bitmap;

    for (size_t i = 0; i < thread_num; ++i) {
        threads.emplace_back([=, &bitmap](){
            bitmap.set(i, true);
        });
    }

    for (auto &&thread : threads) {
        thread.join();
    }

    for (size_t i = 0; i < thread_num; ++i) {
        BOOST_TEST(bitmap.get(i) == true);
    }
}

BOOST_AUTO_TEST_CASE(many_threads_random_keys)
{
    const size_t thread_num = 59;
    setup_logging();
    concurrent_bitmap bitmap;
    std::vector<std::thread> threads;
    std::vector<uint32_t> keys;
    std::srand(42);

    for (size_t i = 0; i < thread_num; ++i) {
        keys.push_back(std::rand() % UINT32_MAX);
    }

    for (size_t i = 0; i < thread_num; ++i) {
        threads.emplace_back([=, &bitmap](){
            uint32_t key = keys[i];
            bitmap.set(key, true);
        });
    }

    for (auto &&thread : threads) {
        thread.join();
    }

    for (size_t i = 0; i < thread_num; ++i) {
        uint32_t key = keys[i];
        BOOST_TEST(bitmap.get(key) == true);
    }
}

BOOST_AUTO_TEST_CASE(many_keys_into_same_leaf_simultaneously)
{
    const size_t thread_count = 4;
    const size_t keys_per_thread = 1024;
    const size_t key_count = thread_count * keys_per_thread;

    setup_logging();
    concurrent_bitmap bitmap;
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


BOOST_AUTO_TEST_SUITE_END() // more_threads