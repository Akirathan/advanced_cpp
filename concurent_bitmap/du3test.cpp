#include <string>
#include <iostream>
#include <cstdlib>
#include <thread>
#include <vector>
#include <mutex>
#include "concurrent_bitmap.h"
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

static vector<pair<uint32_t, uint32_t>> get_keys_that_should_be_in_same_leaf(const vector<uint32_t> &keys)
{
    vector<pair<uint32_t, uint32_t>> keys_in_same_leaf;
    for (size_t i = 0; i < keys.size(); ++i) {
        for (size_t j = i; j < keys.size(); ++j) {
            if (should_be_in_same_leaf(keys[i], keys[j]))
                keys_in_same_leaf.push_back(make_pair(keys[i], keys[j]));
        }
    }
    return keys_in_same_leaf;
}

bool run_test(size_t thread_count, size_t address_base, size_t thread_byte_offset, size_t tested_length)
{
	concurrent_bitmap cbmp;
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

	cbmp.log_bytes_count();
	cbmp.log_nodes_count();
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
		if (!run_test(thread_count, address_base, thread_byte_offset, sample_count))
		{
			cout << "Error" << endl;
			return 0;
		}
	}

	cout << "OK" << endl;

	return 0;
}