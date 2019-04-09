
#include <iostream>
#include <vector>

//uncomment this if you want to use std::allocator instead (and see the result as-is)
//#define USE_STD_ALLOCATOR

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

int main ()
{
#ifndef USE_STD_ALLOCATOR
	std::vector<uint8_t> mem;
	mem.resize (2500000);

	holder::heap (mem.data (), 2500000);
#endif

	for (size_t rep = 0; rep < 1000; ++rep) {

		Vector<int> v; // Constructor of inblock_allocator

		for (size_t i = 0; i < 100000; ++i) v.push_back (i);
		std::cout << v[rep] << std::endl;

		v.clear ();
	}
	return 0;
}
