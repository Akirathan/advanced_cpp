
#include <iostream>
#include <vector>
#include <sys/time.h>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>

//uncomment this if you want to use std::allocator instead (and see the result as-is)
//#define USE_STD_ALLOCATOR


double get_wall_time()
{
    struct timeval time;
    if (gettimeofday(&time, nullptr)){
        //  Handle error
        return 0;
    }
    return (double)time.tv_sec + (double)time.tv_usec * .000001;
}

double get_cpu_time()
{
    return (double)clock() / CLOCKS_PER_SEC;
}

double count_slowdown(double std_time, double my_time)
{
    return my_time / std_time;
}

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

constexpr size_t repetitions = 2; // 1000
constexpr size_t push_backs = 10; // 100000

static void run_myallocator()
{
    for (size_t rep = 0; rep < repetitions; ++rep) {

        Vector<int> v;

        for (size_t i = 0; i < push_backs; ++i) {
            v.push_back (i);
        }

        std::cout << v[rep] << std::endl;

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

        std::cout << v[rep] << std::endl;

        v.clear ();
    }
}

int main ()
{
    init_log();

#ifndef USE_STD_ALLOCATOR
	std::vector<uint8_t> mem;
	mem.resize (2500000);

	holder::heap (mem.data (), 2500000);
#endif

	std::cout << "Repetitions = " << repetitions << std::endl;
	std::cout << "Push backs = " << push_backs << std::endl;

    double my_wall = get_wall_time();

    run_myallocator();

	//  Stop timers
    double my_wall1 = get_wall_time();

    std::cout << "Times for my allocator:" << std::endl;
    std::cout << "\tWall Time = " << my_wall1 - my_wall << std::endl;

    // ==========
    double std_wall = get_wall_time();

    run_stdallocator();

    double std_wall1 = get_wall_time();

    std::cout << "Times for std allocator:" << std::endl;
    std::cout << "\tWall Time = " << std_wall1 - std_wall << std::endl;

    std::cout << "Slowdown of my allocator = " << count_slowdown(std_wall1 - std_wall, my_wall1 - my_wall) << std::endl;
}
