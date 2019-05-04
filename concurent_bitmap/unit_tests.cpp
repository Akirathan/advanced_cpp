#define BOOST_TEST_MODULE My_test

#include <boost/test/included/unit_test.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <cstdlib>
#include <thread>
#include <vector>
#include "concurrent_bitmap.h"

static void setup_logging()
{
    boost::log::core::get()->set_filter
    (
        boost::log::trivial::severity >= boost::log::trivial::info
    );
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
    const size_t thread_num = 19;
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


BOOST_AUTO_TEST_SUITE_END() // more_threads