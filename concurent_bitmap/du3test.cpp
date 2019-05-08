#include <string>
#include <iostream>
#include <cstdlib>
#include <thread>
#include <bitset>
#include <vector>
#include <mutex>
#include "concurrent_bitmap.h"
#include "kuba_concurrent_bitmap.h"
#include "memory_check.hpp"
#include <boost/log/expressions.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>

using namespace std;

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

static bool should_be_in_same_leaf_bitmap(uint32_t key1, uint32_t key2)
{
    std::bitset<32> bitset1{key1};
    std::bitset<32> bitset2{key2};

    // l0, l1, l2 should be same.
    for (size_t i = 0; i < 18; ++i) {
        if (bitset1[i] != bitset2[i])
            return false;
    }
    return true;
}

static vector<pair<uint32_t, uint32_t>> get_keys_that_should_be_in_same_leaf(const vector<uint32_t> &keys)
{
    vector<pair<uint32_t, uint32_t>> keys_in_same_leaf;
    for (size_t i = 0; i < keys.size(); ++i)
        for (size_t j = i+1; j < keys.size(); ++j)
            if (should_be_in_same_leaf_bitmap(keys[i], keys[j]))
                keys_in_same_leaf.emplace_back(make_pair(keys[i], keys[j]));
    return keys_in_same_leaf;
}

static void print_mem_usage()
{
    double vm_usage, resident_set;
    process_mem_usage(vm_usage, resident_set);
    cout << "MEMORY USAGE - VM usage: " << vm_usage << ", resident set: " << resident_set << endl;
}

template <typename BitmapType>
bool run_test(size_t thread_count, size_t address_base, size_t thread_byte_offset, size_t tested_length)
{
	BitmapType cbmp;
	vector<uint32_t> keys;
	mutex keys_mtx;

	thread* threads = new thread[thread_count];
	for (size_t thread_id = 0; thread_id < thread_count; thread_id++)
	{
		threads[thread_id] = thread([=, &cbmp, &keys, &keys_mtx]()
		{
			for (size_t i = 0; i < tested_length; i++)
			{
				// Each thread sets a bit in a particular byte according to its index
				size_t bit_index = address_base + thread_byte_offset * thread_id + i * 8 + thread_id;
				cbmp.set(bit_index, true);
                lock_guard<mutex> lock(keys_mtx);
                keys.push_back(bit_index);
			}
		});
	}

	for (size_t i = 0; i < thread_count; i++)
	{
		threads[i].join();
	}

	// Use only in release.
	//auto keys_in_same_leaf = get_keys_that_should_be_in_same_leaf(keys);
	//cout << "Keys that should be in same leaf cout = " << keys_in_same_leaf.size() << endl;

	print_mem_usage();
	nodes_count node_count = cbmp.get_nodes_count();
	cout << "Nodes count ... inner nodes = " << node_count.inner_nodes_count << ", leaves = " << node_count.leaves_count << endl;

    //cbmp.log_bytes_count();

	for (size_t i = 0; i < tested_length; i++)
	{
		for (size_t thread_id = 0; thread_id < thread_count; thread_id++)
		{
			// Check that the bit was written
			size_t bit_index = address_base + thread_byte_offset * thread_id + i * 8 + thread_id;
			if (!cbmp.get(bit_index))
			{
				//cout << "Error at bit " << hex << bit_index << endl;
				return false;
			}
		}
	}

	delete[] threads;

	//cout << "Test run OK" << endl;
	return true;
}

static bool test_wrapper(size_t thread_count, size_t address_base, size_t thread_byte_offset, size_t sample_count)
{
    bool use_kuba = true;
    if (use_kuba) {
        return run_test<kuba::concurrent_bitmap>(thread_count, address_base, thread_byte_offset, sample_count);
    }
    else {
        return run_test<concurrent_bitmap>(thread_count, address_base, thread_byte_offset, sample_count);
    }
}

int main(int argc, char** argv)
{
    setup_logging();

	size_t thread_count = stoul(argv[1]);
	size_t repeat_count = stoul(argv[2]);
	size_t address_base = stoul(argv[3], nullptr, 16);
	size_t thread_byte_offset = stoul(argv[4], nullptr, 16);
	size_t sample_count = stoul(argv[5], nullptr, 16);

	for (size_t i = 0; i < repeat_count; i++)
	{
		if (!test_wrapper(thread_count, address_base, thread_byte_offset, sample_count))
		{
			cout << "Error" << endl;
			return 0;
		}
	}

	cout << "OK" << endl;

	return 0;
}