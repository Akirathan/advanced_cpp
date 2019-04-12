
#include <iostream>
#include <vector>
#include <sys/time.h>

//uncomment this if you want to use std::allocator instead (and see the result as-is)
//#define USE_STD_ALLOCATOR

#ifdef USE_STD_ALLOCATOR

template<typename V>
using Vector = std::vector<V>;

#else
#include "../inblock_allocator.hpp"

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

struct holder {
	static inblock_allocator_heap heap;
};

inblock_allocator_heap holder::heap;

template<typename V>
using Vector = std::vector<V, inblock_allocator<V, holder>>;
#endif

int main ()
{
#ifndef USE_STD_ALLOCATOR
	std::vector<uint8_t> mem;
	mem.resize (2500000);

	holder::heap (mem.data (), 2500000);
#endif

    double wall0 = get_wall_time();
    double cpu0  = get_cpu_time();

	for (size_t rep = 0; rep < 1000; ++rep) {

		Vector<int> v;

		for (size_t i = 0; i < 100000; ++i) v.push_back (i);
		std::cout << v[rep] << std::endl;

		v.clear ();
	}

	//  Stop timers
    double wall1 = get_wall_time();
    double cpu1  = get_cpu_time();

    std::cout << "Wall Time = " << wall1 - wall0 << std::endl;
    std::cout << "CPU Time  = " << cpu1  - cpu0  << std::endl;
}
