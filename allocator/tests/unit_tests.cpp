
#define BOOST_TEST_MODULE My_test
#include <boost/test/included/unit_test.hpp>

BOOST_AUTO_TEST_CASE(sample_test)
{
    const int value = 3;

    BOOST_TEST(value == 3);
}

