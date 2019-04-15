
#include <cassert>
#include <iostream>
#include <vector>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include "test_common.hpp"

//uncomment this to use std::allocator
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

template<typename V>
using StdVector = std::vector<V>;

using Vec = Vector<int>;
using StdVec = StdVector<int>;
using Matrix = Vector<Vec>;
using StdMatrix = StdVector<StdVec>;


int ugly_dot(Vec a, Vec b)
{
	assert (a.size () == b.size ());
	int res = 0;
	for (size_t i = 0; i < a.size (); ++i) res += a[i] * b[i];
	return res;
}

Matrix ugly_mult_matrix(Matrix a, Matrix b)
{
	Matrix c;
	for (auto&& i : a) {
		Vec tmp;
		for (auto&& j : b) tmp.push_back (ugly_dot (i, j));
		c.push_back (tmp);
	}
	return c;
}

int std_ugly_dot(StdVec a, StdVec b)
{
    assert (a.size () == b.size ());
    int res = 0;
    for (size_t i = 0; i < a.size (); ++i) res += a[i] * b[i];
    return res;
}

StdMatrix std_ugly_mult_matrix(StdMatrix a, StdMatrix b)
{
    StdMatrix c;
    for (auto&& i : a) {
        StdVec tmp;
        for (auto&& j : b) tmp.push_back (std_ugly_dot (i, j));
        c.push_back (tmp);
    }
    return c;
}

#define SIZE 200

#define memsize (SIZE * SIZE * sizeof (int) * 4 * 10)

static void run_myalloc()
{
    Matrix a;
    a.resize (SIZE);
    for (size_t i = 0; i < SIZE; ++i) a[i].resize (SIZE);
    Matrix b = a;

    srand (0x1337);
    for (size_t i = 0; i < SIZE; ++i)
        for (size_t j = 0; j < SIZE; ++j) {
            a[i][j] = rand () % 3;
            b[i][j] = rand () % 3;
        }

    a = ugly_mult_matrix (a, b);
    a = ugly_mult_matrix (a, b);
    a = ugly_mult_matrix (a, b);

    for (size_t i = 0; i < SIZE; ++i) {
        for (size_t j = 0; j < SIZE; ++j) {
            //std::cout << a[i][j] << ' ';
        }
        //std::cout << std::endl;
    }
}

static void run_stdalloc()
{
    StdMatrix a;
    a.resize (SIZE);
    for (size_t i = 0; i < SIZE; ++i) a[i].resize (SIZE);
    StdMatrix b = a;

    srand (0x1337);
    for (size_t i = 0; i < SIZE; ++i)
        for (size_t j = 0; j < SIZE; ++j) {
            a[i][j] = rand () % 3;
            b[i][j] = rand () % 3;
        }

    a = std_ugly_mult_matrix (a, b);
    a = std_ugly_mult_matrix (a, b);
    a = std_ugly_mult_matrix (a, b);

    for (size_t i = 0; i < SIZE; ++i) {
        for (size_t j = 0; j < SIZE; ++j) {
            //std::cout << a[i][j] << ' ';
        }
        //std::cout << std::endl;
    }
}

int main ()
{
    boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::warning);

#ifndef USE_STD_ALLOCATOR
	std::vector<uint8_t> mem;
	mem.resize (memsize);

	holder::heap (mem.data (), memsize);
#endif

    double my_wall_time = measure(run_myalloc);

    std::cout << "Times for my allocator:" << std::endl;
    std::cout << "\tWall Time = " << my_wall_time << std::endl;

    // ==========
    double std_wall_time = measure(run_stdalloc);

    std::cout << "Times for std allocator:" << std::endl;
    std::cout << "\tWall Time = " << std_wall_time << std::endl;

    std::cout << "Slowdown of my allocator = " << count_slowdown(std_wall_time, my_wall_time) << std::endl;

}
