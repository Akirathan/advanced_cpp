#include <string>
#include <iostream>
#include <thread>
#include "concurrent_bitmap.h"

using namespace std;

bool run_test(size_t thread_count, size_t address_base, size_t thread_byte_offset, size_t tested_length)
{
	concurrent_bitmap cbmp;

	thread* threads = new thread[thread_count];
	for (size_t thread_id = 0; thread_id < thread_count; thread_id++)
	{
		threads[thread_id] = thread([=, &cbmp]()
		{
			for (size_t i = 0; i < tested_length; i++)
			{
				// Each thread sets a bit in a particular byte according to its index
				size_t bit_index = address_base + thread_byte_offset * thread_id + i * 8 + thread_id;
				cbmp.set(bit_index, true);
			}
		});
	}

	for (size_t i = 0; i < thread_count; i++)
	{
		threads[i].join();
	}

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