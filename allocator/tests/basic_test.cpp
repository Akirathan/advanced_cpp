
#include <iostream>
#include <vector>
#include <sys/time.h>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include "test_common.hpp"

//uncomment this if you want to use std::allocator instead (and see the result as-is)
//#define USE_STD_ALLOCATOR


static void init_log()
{
    boost::log::core::get()->set_filter
    (
        boost::log::trivial::severity >= boost::log::trivial::info
    );
}

#ifdef USE_STD_ALLOCATOR

template<typename V>
using Vector = std::vector<V>;

#else
#include "../inblock_allocator.hpp"

struct holder {
	static inblock_allocator_heap heap;
};

inblock_allocator_heap holder::heap;

template<typename V>
using Vector = std::vector<V, inblock_allocator<V, holder>>;
#endif

constexpr size_t repetitions = 10000; // 1000
constexpr size_t push_backs = 100000; // 100000

static void run_myallocator()
{
    for (size_t rep = 0; rep < repetitions; ++rep) {

        Vector<int> v;

        for (size_t i = 0; i < push_backs; ++i) {
            v.push_back (i);
        }

        //std::cout << v[rep] << std::endl;

        v.clear ();
    }
}

static void run_stdallocator()
{
    for (size_t rep = 0; rep < repetitions; ++rep) {

        std::vector<int> v;

        for (size_t i = 0; i < push_backs; ++i) {
            v.push_back (i);
        }

        //std::cout << v[rep] << std::endl;

        v.clear ();
    }
}


int main ()
{
    init_log();

#ifndef USE_STD_ALLOCATOR
	std::vector<uint8_t> mem;
	mem.resize(2500000);

	holder::heap(mem.data (), 2500000);
#endif

	std::cout << "Repetitions = " << repetitions << std::endl;
	std::cout << "Push backs = " << push_backs << std::endl;

    double my_wall_time = measure(run_myallocator);

    std::cout << "Times for my allocator:" << std::endl;
    std::cout << "\tWall Time = " << my_wall_time << std::endl;

    // ==========
    double std_wall_time = measure(run_stdallocator);

    std::cout << "Times for std allocator:" << std::endl;
    std::cout << "\tWall Time = " << std_wall_time << std::endl;

    std::cout << "Slowdown of my allocator = " << count_slowdown(std_wall_time, my_wall_time) << std::endl;
}
