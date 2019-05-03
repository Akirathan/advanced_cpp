#define BOOST_TEST_MODULE My_test

#include <boost/test/included/unit_test.hpp>
#include "concurrent_bitmap.h"

BOOST_AUTO_TEST_SUITE(one_thread)

BOOST_AUTO_TEST_CASE(simple_test)
{
    concurrent_bitmap bitmap;
    uint32_t key = 0;
    bitmap.set(key, true);
    BOOST_TEST(bitmap.get(key) == true);
}

BOOST_AUTO_TEST_CASE(double_get)
{
    concurrent_bitmap bitmap;
    uint32_t key = 6;
    bitmap.set(key, true);
    BOOST_TEST(bitmap.get(key) == true);
    BOOST_TEST(bitmap.get(key) == true);
}

BOOST_AUTO_TEST_CASE(get_fails)
{
    concurrent_bitmap bitmap;
    bitmap.set(542, true);
    BOOST_TEST(bitmap.get(14) == false);
}

BOOST_AUTO_TEST_CASE(more_sets)
{
    concurrent_bitmap bitmap;
    for (uint32_t key = 40; key < 500; key += 7) {
        bitmap.set(key, true);
    }

    for (uint32_t key = 40; key < 500; key += 7) {
        BOOST_TEST(bitmap.get(key) == true);
    }
}

BOOST_AUTO_TEST_SUITE_END() // one_thread
