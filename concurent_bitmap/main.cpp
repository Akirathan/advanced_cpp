#include "concurrent_bitmap.h"
#include "kuba_concurrent_bitmap.h"
#include <thread>
#include <vector>
#include <bitset>

static void kuba_bitmap()
{
    const size_t repetitions = 3;
    const size_t keys_per_thread_count = 100;
    const size_t thread_count = 8;


    for (size_t repetition = 0; repetition < repetitions; ++repetition) {
        kuba::concurrent_bitmap bitmap;
        std::vector<std::thread> threads;

        for (size_t thread_idx = 0; thread_idx < thread_count; thread_idx++) {
            threads.emplace_back([=, &bitmap](){
                for (size_t i = thread_idx; i < thread_idx + keys_per_thread_count; i++) {
                    bitmap.set(i, true);
                }
            });
        }

        for (std::thread &thread : threads) {
            thread.join();
        }
    }
}

static void my_bitmap()
{
    const size_t repetitions = 3;
    const size_t keys_per_thread_count = 100;
    const size_t thread_count = 8;


    for (size_t repetition = 0; repetition < repetitions; ++repetition) {
        concurrent_bitmap bitmap;
        std::vector<std::thread> threads;

        for (size_t thread_idx = 0; thread_idx < thread_count; thread_idx++) {
            threads.emplace_back([=, &bitmap](){
                for (size_t i = thread_idx; i < thread_idx + keys_per_thread_count; i++) {
                    bitmap.set(i, true);
                }
            });
        }

        for (std::thread &thread : threads) {
            thread.join();
        }
    }
}

static void kuba_empty_test()
{
    kuba::concurrent_bitmap bitmap;
    assert(bitmap.get_set_bytes() == 0);
}

static void uint_max_test()
{
    uint32_t val = UINT32_MAX;
    std::bitset<32> bitset{val};
    for (size_t i = 0; i < 32; ++i) {
        if (i % 4 == 0) {
            std::cout << " ";
        }
        if (i % 8 == 0) {
            std::cout << "-";
        }
        std::cout << bitset[i];
    }
    std::cout << std::endl;
}

static void simple_kuba_test()
{
    const int key_count = 1;
    kuba::concurrent_bitmap bitmap;
    for (int i = 0; i < key_count; i++) {
        uint32_t key = std::rand() % UINT32_MAX;
        bitmap.set(key, true);
    }
    std::cout << "Set bytes = " << bitmap.get_set_bytes() << std::endl;
}

int main()
{
    uint_max_test();
}
