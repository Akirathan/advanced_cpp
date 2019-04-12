
#define BOOST_TEST_MODULE My_test

#include <array>
#include <memory>
#include <iostream>
#include <functional>
#include <boost/test/included/unit_test.hpp>
#include "../inblock_allocator.hpp"
#include "../small_bins.hpp"
#include "../chunk_list.hpp"
#include "../common.hpp"
#include "../chunk.hpp"

static constexpr uint8_t MAGIC = 0xA3;

static constexpr size_t memory_size = 5 * 1024 * 1024;
static std::array<uint8_t, memory_size> memory = {};


static void fill_payload(void *payload, size_t size)
{
    uint8_t *ptr = reinterpret_cast<uint8_t *>(payload);
    for (size_t i = 0; i < size; i++) {
        *ptr = MAGIC;
        ptr++;
    }
}

static bool check_payload_consistency(void *payload, size_t size)
{
    uint8_t *ptr = reinterpret_cast<uint8_t *>(payload);
    for (size_t i = 0; i < size; ++i) {
        if (*ptr != MAGIC) {
            return false;
        }
        ptr++;
    }
    return true;
}

static bool is_payload_aligned(const chunk_t *chunk)
{
    assert(chunk);
    void *data_ptr = get_chunk_data(chunk);
    return is_aligned((intptr_t)data_ptr);
}

static bool is_chunk_in_initialized_state(const chunk_t *chunk)
{
    return chunk->payload_size > 0 && chunk->used == 0;
}

static void fill_memory_region_with_random_data(intptr_t start_addr, intptr_t end_addr)
{
    while (start_addr < end_addr) {
        uint8_t *byte_ptr = reinterpret_cast<uint8_t *>(start_addr);
        *byte_ptr = static_cast<uint8_t>(rand());
        start_addr++;
    }
}

static void traverse_all_memory(intptr_t start_addr, intptr_t end_addr, std::function<void(chunk_t *)> func)
{
    chunk_t *chunk = reinterpret_cast<chunk_t *>(start_addr);
    while (start_addr != end_addr) {
        start_addr += get_chunk_size(chunk);
        func(chunk);
        chunk = next_chunk_in_mem(chunk);
    }
}

static size_t count_used_chunks(intptr_t start_addr, intptr_t end_addr)
{
    size_t used_chunks = 0;
    traverse_all_memory(start_addr, end_addr, [&used_chunks](chunk_t *chunk) {
        if (chunk->used) {
            used_chunks++;
        }
    });
    return used_chunks;
}

static size_t count_free_chunks(intptr_t start_addr, intptr_t end_addr)
{
    size_t free_chunks = 0;
    traverse_all_memory(start_addr, end_addr, [&free_chunks](chunk_t *chunk) {
        if (!chunk->used) {
            free_chunks++;
        }
    });
    return free_chunks;
}

static void check_memory_filled_with_chunks(intptr_t start_addr, intptr_t end_addr)
{
    traverse_all_memory(start_addr, end_addr, [](chunk_t *chunk) {
        BOOST_TEST(is_chunk_in_initialized_state(chunk));
        BOOST_TEST(is_payload_aligned(chunk));
    });
}

static double count_chunk_headers_overhead(intptr_t start_addr, intptr_t end_addr)
{
    size_t total_memory = diff(start_addr, end_addr);

    size_t chunk_headers_size_sum = 0;
    traverse_all_memory(start_addr, end_addr, [&](chunk_t *) {
        chunk_headers_size_sum += chunk_header_size;
    });

    return ((double)chunk_headers_size_sum / (double)total_memory);
}


std::pair<intptr_t, intptr_t> get_aligned_memory_region(size_t size)
{
    assert(size % alignment == 0);
    assert(size < memory_size);

    intptr_t start_addr = reinterpret_cast<intptr_t>(memory.data());
    while (!is_aligned(start_addr)) {
        start_addr++;
    }

    intptr_t end_addr = start_addr + size;

    return std::make_pair(start_addr, end_addr);
}


static bool equals_some(const chunk_t *chunk, const std::vector<chunk_t *> &chunks)
{
    for (const chunk_t *chunk_item : chunks) {
        if (chunk_item == chunk) {
            return true;
        }
    }
    return false;
}

static bool is_linked(const chunk_t *first_chunk, const chunk_t *second_chunk)
{
    return first_chunk->next == second_chunk && second_chunk->prev == first_chunk;
}

static bool is_correct_list(const ChunkList &chunk_list)
{
    const chunk_t *first_chunk = chunk_list.get_first_chunk();
    chunk_t *second_chunk = first_chunk->next;
    chunk_t *chunk = chunk_list.get_first_chunk();
    while (second_chunk && second_chunk != first_chunk) {
        if (!is_linked(chunk, second_chunk)) {
            return false;
        }

        chunk = second_chunk;
        second_chunk = second_chunk->next;
    }
    return true;
}

static void dump_bin_sizes(const SmallBins &small_bins)
{
    for (size_t chunk_size = small_bins.min_chunk_size_for_bins;
        chunk_size <= small_bins.max_chunk_size_for_bins;
        chunk_size += alignment)
    {
        assert(small_bins.contains_bin_with_chunk_size(chunk_size));
        size_t bin_size = small_bins.get_bin_size(chunk_size);

        BOOST_TEST_MESSAGE("Bin with chunk sizes " << chunk_size << " contains " << bin_size << " chunks.");
    }
}


static std::vector<std::unique_ptr<chunk_t>> create_chunks(size_t count)
{
    std::vector<std::unique_ptr<chunk_t>> chunk_vector;
    for (size_t i = 0; i < count; ++i) {
        chunk_vector.emplace_back(std::make_unique<chunk_t>());
    }
    return chunk_vector;
}

static SmallBins initialize_small_bins(size_t mem_size, intptr_t *start_addr_out, intptr_t *end_addr_out)
{
    auto [start_addr, end_addr] = get_aligned_memory_region(mem_size);
    fill_memory_region_with_random_data(start_addr, end_addr);

    SmallBins small_bins;
    intptr_t returned_addr = small_bins.initialize_memory(start_addr, end_addr);
    BOOST_TEST(returned_addr <= end_addr);
    BOOST_TEST_MESSAGE("Small bins initialized with " << mem_size << " discarded " << diff(returned_addr, end_addr));
    BOOST_TEST_MESSAGE("Chunk headers memory overhead = " << count_chunk_headers_overhead(start_addr, returned_addr));
    dump_bin_sizes(small_bins);

    if (start_addr_out && end_addr_out) {
        *start_addr_out = start_addr;
        *end_addr_out = returned_addr;
    }
    return small_bins;
}

/* ===================================================================================================== */
/* ============================== CHUNK LIST TESTS ===================================================== */
/* ===================================================================================================== */

BOOST_AUTO_TEST_CASE(chunk_list_prepend_test)
{
    auto first_chunk_storage = std::make_unique<chunk_t>();
    auto second_chunk_storage = std::make_unique<chunk_t>();
    chunk_t *first_chunk = first_chunk_storage.get();
    chunk_t *second_chunk = second_chunk_storage.get();

    ChunkList chunk_list;
    chunk_list.prepend_chunk(second_chunk);
    chunk_list.prepend_chunk(first_chunk);

    BOOST_TEST(is_correct_list(chunk_list));
    BOOST_TEST(chunk_list.get_first_chunk() == first_chunk);
}

BOOST_AUTO_TEST_CASE(chunk_list_append_simple_test)
{
    auto first_chunk_storage = std::make_unique<chunk_t>();
    auto second_chunk_storage = std::make_unique<chunk_t>();
    chunk_t *first_chunk = first_chunk_storage.get();
    chunk_t *second_chunk = second_chunk_storage.get();

    ChunkList chunk_list;
    chunk_list.append_chunk(first_chunk);
    chunk_list.append_chunk(second_chunk);

    BOOST_TEST(is_correct_list(chunk_list));
    BOOST_TEST(chunk_list.get_first_chunk() == first_chunk);
}

BOOST_AUTO_TEST_CASE(chunk_list_append_more_chunks_test)
{
    auto first_chunk_storage = std::make_unique<chunk_t>();
    chunk_t *first_chunk = first_chunk_storage.get();
    auto chunk_vector = create_chunks(20);

    ChunkList chunk_list{first_chunk};

    for (auto &chunk_ptr : chunk_vector) {
        chunk_list.append_chunk(chunk_ptr.get());
    }

    BOOST_TEST(chunk_list.size() == chunk_vector.size() + 1);
    BOOST_TEST(chunk_list.get_first_chunk() == first_chunk);
}

BOOST_AUTO_TEST_CASE(chunk_list_is_empty_test)
{
    auto first_chunk_storage = std::make_unique<chunk_t>();
    auto second_chunk_storage = std::make_unique<chunk_t>();
    chunk_t *first_chunk = first_chunk_storage.get();
    chunk_t *second_chunk = second_chunk_storage.get();

    ChunkList chunk_list;
    BOOST_TEST(chunk_list.is_empty());

    chunk_list.prepend_chunk(first_chunk);
    BOOST_TEST(!chunk_list.is_empty());

    chunk_list.prepend_chunk(second_chunk);
    BOOST_TEST(!chunk_list.is_empty());
}

BOOST_AUTO_TEST_CASE(chunk_list_small_size_test)
{
    auto first_chunk_storage = std::make_unique<chunk_t>();
    chunk_t *first_chunk = first_chunk_storage.get();

    ChunkList chunk_list;
    BOOST_TEST(chunk_list.size() == 0);

    chunk_list.prepend_chunk(first_chunk);
    BOOST_TEST(chunk_list.size() == 1);
}

BOOST_AUTO_TEST_CASE(chunk_list_bigger_size_test)
{
    const size_t chunk_count = 13;
    auto chunk_vector = create_chunks(chunk_count);

    ChunkList chunk_list;
    for (auto &chunk_ptr : chunk_vector) {
        chunk_list.prepend_chunk(chunk_ptr.get());
    }

    BOOST_TEST(chunk_list.size() == chunk_count);
}

BOOST_AUTO_TEST_CASE(chunk_list_pop_first_test)
{
    auto first_chunk_storage = std::make_unique<chunk_t>();
    auto second_chunk_storage = std::make_unique<chunk_t>();
    chunk_t *first_chunk = first_chunk_storage.get();
    chunk_t *second_chunk = second_chunk_storage.get();

    ChunkList chunk_list;
    chunk_list.prepend_chunk(second_chunk);
    chunk_list.prepend_chunk(first_chunk);

    chunk_t *popped_chunk = chunk_list.pop_first_chunk();
    BOOST_TEST(popped_chunk == first_chunk);

    popped_chunk = chunk_list.pop_first_chunk();
    BOOST_TEST(popped_chunk == second_chunk);

    BOOST_TEST(chunk_list.is_empty());
}

BOOST_AUTO_TEST_CASE(chunk_list_find_free_chunk_test)
{
    auto first_chunk_storage = std::make_unique<chunk_t>();
    auto second_chunk_storage = std::make_unique<chunk_t>();
    chunk_t *first_chunk = first_chunk_storage.get();
    chunk_t *second_chunk = second_chunk_storage.get();

    ChunkList chunk_list;
    BOOST_TEST(!chunk_list.find_free_chunk());

    chunk_list.prepend_chunk(second_chunk);
    BOOST_TEST(chunk_list.find_free_chunk());

    chunk_list.prepend_chunk(first_chunk);
    BOOST_TEST(chunk_list.find_free_chunk());
}

BOOST_AUTO_TEST_CASE(chunk_list_remove_simple_test)
{
    auto first_chunk_storage = std::make_unique<chunk_t>();
    chunk_t *first_chunk = first_chunk_storage.get();

    ChunkList chunk_list{first_chunk};

    chunk_list.remove_chunk(first_chunk);
    BOOST_TEST(chunk_list.is_empty());
}

BOOST_AUTO_TEST_CASE(chunk_list_remove_more_chunks_test)
{
    auto chunk_vector = create_chunks(23);

    ChunkList chunk_list;
    for (auto &chunk_ptr : chunk_vector) {
        chunk_list.prepend_chunk(chunk_ptr.get());
    }

    // Remove backwards.
    for (auto it = chunk_vector.rbegin(); it != chunk_vector.rend(); it++) {
        chunk_list.remove_chunk(it->get());
    }

    BOOST_TEST(chunk_list.is_empty());
}

BOOST_AUTO_TEST_CASE(chunk_list_remove_first_chunk_test)
{
    auto first_chunk = std::make_unique<chunk_t>();
    auto second_chunk = std::make_unique<chunk_t>();

    ChunkList chunk_list{first_chunk.get()};
    chunk_list.append_chunk(second_chunk.get());
    BOOST_TEST(chunk_list.size() == 2);

    chunk_list.remove_chunk(first_chunk.get());
    BOOST_TEST(chunk_list.size() == 1);
}

BOOST_AUTO_TEST_CASE(chunk_list_try_remove_test)
{
    size_t chunk_count = 15;
    auto chunk_vector = create_chunks(chunk_count);

    ChunkList chunk_list;
    for (auto &chunk_ptr : chunk_vector) {
        chunk_list.prepend_chunk(chunk_ptr.get());
    }
    BOOST_TEST(chunk_list.size() == chunk_count);

    chunk_t *first_chunk = chunk_list.get_first_chunk();
    BOOST_TEST(chunk_list.try_remove_chunk(first_chunk));
    BOOST_TEST(chunk_list.size() == chunk_count - 1);
    chunk_t *new_first_chunk = chunk_list.get_first_chunk();
    BOOST_TEST(first_chunk != new_first_chunk);

    chunk_t *chunk_in_middle = new_first_chunk->next->next->next;
    BOOST_TEST(chunk_list.try_remove_chunk(chunk_in_middle));
    BOOST_TEST(chunk_list.size() == chunk_count - 2);

    auto completely_different_chunk = std::make_unique<chunk_t>();
    BOOST_TEST(!chunk_list.try_remove_chunk(completely_different_chunk.get()));
}

BOOST_AUTO_TEST_CASE(chunk_list_find_and_remove_test)
{
    auto chunk = std::make_unique<chunk_t>();
    ChunkList chunk_list{chunk.get()};
    chunk_t *free_chunk = chunk_list.find_free_chunk();
    BOOST_TEST(free_chunk);
    chunk_list.remove_chunk(free_chunk);

    BOOST_TEST(chunk_list.is_empty());
}

BOOST_AUTO_TEST_CASE(chunk_list_pop_chunk_with_size_at_least_simple_test)
{
    auto chunk = std::make_unique<chunk_t>();
    chunk->payload_size = 50;

    ChunkList chunk_list{chunk.get()};

    chunk_t *popped_chunk = chunk_list.pop_chunk_with_size_at_least(20);
    BOOST_TEST(popped_chunk == chunk.get());
}

BOOST_AUTO_TEST_CASE(chunk_list_pop_chunk_with_size_at_least_first_chunk_test)
{
    std::vector<std::unique_ptr<chunk_t>> chunk_vector;
    for (size_t i = 0; i < 5; ++i) {
        chunk_vector.emplace_back(std::make_unique<chunk_t>());
    }

    chunk_vector[0]->payload_size = 13;
    chunk_vector[1]->payload_size = 15;
    chunk_vector[2]->payload_size = 42;
    chunk_vector[3]->payload_size = 3;
    chunk_vector[4]->payload_size = 5;

    ChunkList chunk_list;
    for (auto &chunk_ptr : chunk_vector) {
        chunk_list.append_chunk(chunk_ptr.get());
    }

    chunk_t *first_chunk = chunk_list.pop_chunk_with_size_at_least(8);
    BOOST_TEST(first_chunk);
    BOOST_TEST(first_chunk->payload_size == 13);
}

BOOST_AUTO_TEST_CASE(chunk_list_pop_chunk_with_size_at_least_test)
{
    std::vector<std::unique_ptr<chunk_t>> chunk_vector;
    for (size_t i = 0; i < 5; ++i) {
        chunk_vector.emplace_back(std::make_unique<chunk_t>());
    }

    chunk_vector[0]->payload_size = 13;
    chunk_vector[1]->payload_size = 15;
    chunk_vector[2]->payload_size = 42;
    chunk_vector[3]->payload_size = 3;
    chunk_vector[4]->payload_size = 5;

    ChunkList chunk_list;
    for (auto &chunk_ptr : chunk_vector) {
        chunk_list.append_chunk(chunk_ptr.get());
    }

    chunk_t *chunk = chunk_list.pop_chunk_with_size_at_least(40);
    BOOST_TEST(chunk);
    BOOST_TEST(chunk->payload_size >= 40);
    BOOST_TEST(chunk_list.size() == 4);

    chunk = chunk_list.pop_chunk_with_size_at_least(10);
    BOOST_TEST(chunk);
    BOOST_TEST(chunk->payload_size >= 10);
    BOOST_TEST(chunk_list.size() == 3);
}

/* ===================================================================================================== */
/* ============================== CHUNK TESTS ===================================================== */
/* ===================================================================================================== */
/**
 * Initialize two chunks by hand a fill them with some magic payload. Then check
 * for consistency of those payloads.
 */
BOOST_AUTO_TEST_CASE(two_chunks_payloads_consistency_test)
{
    std::array<uint8_t, 100> mem = {};
    const size_t payload_size = 16;
    intptr_t start_addr = reinterpret_cast<intptr_t>(mem.data());
    chunk_t *first_chunk = initialize_chunk(start_addr, payload_size);
    fill_payload(get_chunk_data(first_chunk), payload_size);

    intptr_t next_addr = start_addr + get_chunk_size(first_chunk);
    chunk_t *second_chunk = initialize_chunk(next_addr, payload_size);
    fill_payload(get_chunk_data(second_chunk), payload_size);

    // Check payload of both chunks.
    BOOST_TEST(check_payload_consistency(get_chunk_data(first_chunk), payload_size));
    BOOST_TEST(check_payload_consistency(get_chunk_data(second_chunk), payload_size));
}

BOOST_AUTO_TEST_CASE(chunk_one_split_test)
{
    const size_t mem_size = 160;
    auto [start_addr, end_addr] = get_aligned_memory_region(mem_size);

    // Chunk is over whole memory.
    size_t max_payload_size = mem_size - chunk_header_size;
    chunk_t *chunk = initialize_chunk(start_addr, max_payload_size);
    BOOST_TEST(chunk);

    chunk_t *new_chunk = split_chunk(chunk, 16);
    BOOST_TEST(new_chunk);

    // Memory should be filled with two chunks.
    chunk_t *next_chunk = next_chunk_in_mem(chunk);
    BOOST_TEST(next_chunk == new_chunk);

    // Chunk's payload should be decreased.
    BOOST_TEST(chunk->payload_size < max_payload_size);
}

/**
 * Splits one huge chunk into more chunks and checks consistency.
 */
BOOST_AUTO_TEST_CASE(chunk_more_splits_test)
{
    const size_t mem_size = 1024;
    auto [start_addr, end_addr] = get_aligned_memory_region(mem_size);

    size_t max_payload_size = mem_size - chunk_header_size;
    chunk_t *first_chunk = initialize_chunk(start_addr, max_payload_size);
    BOOST_TEST(first_chunk);

    std::vector<chunk_t *> new_chunks;
    const size_t new_chunk_payload_size = 16;
    const size_t number_of_splits = 3;
    for (size_t i = 0; i < number_of_splits; ++i) {
        chunk_t *new_chunk = split_chunk(first_chunk, new_chunk_payload_size);
        BOOST_TEST(new_chunk);
        new_chunks.push_back(new_chunk);
    }

    chunk_t *chunk_in_memory = first_chunk;
    for (size_t i = 0; i < number_of_splits; ++i) {
        chunk_in_memory = next_chunk_in_mem(chunk_in_memory);
        BOOST_TEST(equals_some(chunk_in_memory, new_chunks));
    }
}

BOOST_AUTO_TEST_CASE(chunk_join_test)
{
    const size_t mem_size = 1024;
    auto [start_addr, end_addr] = get_aligned_memory_region(mem_size);
    chunk_t *first_chunk = nullptr;
    chunk_t *second_chunk = nullptr;

    first_chunk = initialize_chunk(start_addr, min_payload_size);
    second_chunk = initialize_chunk(start_addr + get_chunk_size(first_chunk), min_payload_size);

    join_chunks(first_chunk, second_chunk);
    BOOST_TEST(first_chunk->payload_size > min_payload_size);
}

/* ===================================================================================================== */
/* ============================== HEAP TESTS ===================================================== */
/* ===================================================================================================== */
BOOST_AUTO_TEST_CASE(aligned_heap_test)
{
    std::array<uint8_t, 100> mem = {};
    inblock_allocator_heap heap;
    heap((void *)mem.data(), 100);

    BOOST_TEST(is_aligned(heap.get_start_addr()));
    BOOST_TEST(is_aligned(heap.get_end_addr()));
    BOOST_TEST(heap.get_size() <= 100);
}


/* ===================================================================================================== */
/* ============================== SMALL BINS TESTS ===================================================== */
/* ===================================================================================================== */
BOOST_AUTO_TEST_CASE(small_bins_memory_initialization)
{
    auto [start_addr, end_addr] = get_aligned_memory_region(1024);
    fill_memory_region_with_random_data(start_addr, end_addr);

    SmallBins small_bins;
    intptr_t returned_addr = small_bins.initialize_memory(start_addr, end_addr);

    BOOST_TEST(returned_addr <= end_addr);

    traverse_all_memory(start_addr, returned_addr, [](chunk_t *chunk) {
        BOOST_TEST(is_chunk_in_initialized_state(chunk));
        BOOST_TEST(is_payload_aligned(chunk));
    });
}

BOOST_AUTO_TEST_CASE(small_bins_simple_allocation_test)
{
    SmallBins small_bins = initialize_small_bins(80, nullptr, nullptr);

    chunk_t *allocated_chunk = small_bins.allocate_chunk(SmallBins::min_chunk_size_for_bins);
    BOOST_TEST(allocated_chunk);
    BOOST_TEST(allocated_chunk->payload_size >= SmallBins::min_chunk_size_for_bins);
    BOOST_TEST(is_payload_aligned(allocated_chunk));
    BOOST_TEST(is_chunk_in_initialized_state(allocated_chunk));
}

BOOST_AUTO_TEST_CASE(small_bins_allocation_failed_test)
{
    SmallBins small_bins = initialize_small_bins(80, nullptr, nullptr);

    chunk_t *allocated_chunk = small_bins.allocate_chunk(SmallBins::max_chunk_size_for_bins);
    BOOST_TEST(allocated_chunk == nullptr);
}

BOOST_AUTO_TEST_CASE(small_bins_split_bigger_chunk_allocation_test)
{
    SmallBins small_bins = initialize_small_bins(1024, nullptr, nullptr);

    // Allocate all chunks from smallest bin.
    size_t smallest_payload_size = SmallBins::min_chunk_size_for_bins;
    size_t min_bin_size = small_bins.get_bin_size(smallest_payload_size);
    for (size_t i = 0; i < min_bin_size; i++) {
        chunk_t *chunk = small_bins.allocate_chunk(smallest_payload_size);
        BOOST_TEST(chunk);
    }
    BOOST_TEST_MESSAGE("Bin sizes after allocation of all smallest chunks:");
    dump_bin_sizes(small_bins);

    // Try to allocate one more chunk from smallest bin - small bins should split bigger chunk.
    chunk_t *chunk = small_bins.allocate_chunk(smallest_payload_size);
    BOOST_TEST(chunk);
    BOOST_TEST(chunk->payload_size >= smallest_payload_size);
    BOOST_TEST_MESSAGE("Bin sizes after allocation of one more smallest chunk (should split bigger chunk):");
    dump_bin_sizes(small_bins);
}


/* ===================================================================================================== */
/* ============================== LARGE BIN TESTS ===================================================== */
/* ===================================================================================================== */

BOOST_AUTO_TEST_CASE(large_bin_memory_init_test)
{
    auto [start_addr, end_addr] = get_aligned_memory_region(LargeBin::min_chunk_size * 10);
    fill_memory_region_with_random_data(start_addr, end_addr);

    LargeBin large_bin;
    intptr_t returned_addr = large_bin.initialize_memory(start_addr, end_addr);

    BOOST_TEST(returned_addr <= end_addr);

    traverse_all_memory(start_addr, returned_addr, [](chunk_t *chunk) {
        BOOST_TEST(is_chunk_in_initialized_state(chunk));
        BOOST_TEST(is_payload_aligned(chunk));
    });
}


/* ===================================================================================================== */
/* ============================== ALLOCATOR TESTS ===================================================== */
/* ===================================================================================================== */

struct holder {
    static inblock_allocator_heap heap;
};

inblock_allocator_heap holder::heap;

struct allocator_stats_t {
    intptr_t used_mem_start;
    intptr_t used_mem_end;
    size_t available_mem_size;
    size_t used_mem_size;
    size_t large_bin_chunk_count;
    size_t large_bin_total_chunk_size;
    size_t small_bins_chunk_count;
    size_t small_bins_total_chunk_size;
};

template <typename T, typename HeapHolder>
allocator_stats_t get_allocator_stats(const inblock_allocator<T, HeapHolder> &allocator)
{
    allocator_stats_t stats;
    stats.used_mem_start = allocator.get_chunk_region_start_addr();
    stats.used_mem_end = allocator.get_chunk_region_end_addr();
    stats.available_mem_size = diff(holder::heap.get_start_addr(), holder::heap.get_end_addr());
    stats.used_mem_size = diff(stats.used_mem_start, stats.used_mem_end);

    const ChunkList &large_bin_chunk_list = allocator.get_large_bin().get_chunk_list();
    stats.large_bin_chunk_count = large_bin_chunk_list.size();
    stats.large_bin_total_chunk_size = 0;
    const_cast<ChunkList &>(large_bin_chunk_list).traverse([&](const chunk_t *chunk) {
        stats.large_bin_total_chunk_size += chunk->payload_size;
    });

    // Get small bins stats.
    const SmallBins &small_bins = allocator.get_small_bins();
    stats.small_bins_chunk_count = 0;
    stats.small_bins_total_chunk_size = 0;
    for (size_t chunk_size = SmallBins::min_chunk_size_for_bins;
         chunk_size <= SmallBins::max_chunk_size_for_bins;
         chunk_size += alignment)
    {
        assert(small_bins.contains_bin_with_chunk_size(chunk_size));
        size_t bin_size = small_bins.get_bin_size(chunk_size);
        stats.small_bins_chunk_count += bin_size;
        stats.small_bins_total_chunk_size += bin_size * chunk_size;
    }

    return stats;
}

static void dump_allocator_stats(const allocator_stats_t &stats)
{
    BOOST_TEST_MESSAGE("=========== ALLOCATOR STATS ===================");
    BOOST_TEST_MESSAGE("MEMORY USAGE:");
    BOOST_TEST_MESSAGE("\t Available memory size = " << stats.available_mem_size);
    BOOST_TEST_MESSAGE("\t Used memory size = " << stats.used_mem_size);
    BOOST_TEST_MESSAGE("\t Difference = " << stats.available_mem_size - stats.used_mem_size);

    BOOST_TEST_MESSAGE("CHUNK HEADERS OVERHEAD:");
    BOOST_TEST_MESSAGE("\t " << count_chunk_headers_overhead(stats.used_mem_start, stats.used_mem_end));

    BOOST_TEST_MESSAGE("==============================");
    BOOST_TEST_MESSAGE("LARGE BIN STATS:");
    BOOST_TEST_MESSAGE("\t Number of chunks: " << stats.large_bin_chunk_count);
    BOOST_TEST_MESSAGE("\t Total size of chunks: " << stats.large_bin_total_chunk_size);

    BOOST_TEST_MESSAGE("==============================");
    BOOST_TEST_MESSAGE("SMALL BIN STATS:");
    BOOST_TEST_MESSAGE("\t Number of chunks: " << stats.small_bins_chunk_count);
    BOOST_TEST_MESSAGE("\t Total size of chunks: " << stats.small_bins_total_chunk_size);

    BOOST_TEST_MESSAGE("============= END OF ALLOCATOR STATS =================");
}

static void check_allocator_stats(const allocator_stats_t &stats)
{
    BOOST_TEST(stats.used_mem_size <= stats.available_mem_size);
}

static void check_allocator_consistency(const allocator_stats_t &stats)
{
    size_t reachable_chunks_by_allocator = stats.small_bins_chunk_count + stats.large_bin_chunk_count;

    size_t total_chunks_in_memory = 0;
    size_t used_chunks = 0;
    traverse_all_memory(stats.used_mem_start, stats.used_mem_end, [&](chunk_t *chunk) {
        total_chunks_in_memory++;
        if (chunk->used) {
            used_chunks++;
        }
    });

    BOOST_TEST_MESSAGE("Number of reachable chunks by allocator: " << reachable_chunks_by_allocator);
    BOOST_TEST_MESSAGE("Number of total chunks in memory: " << total_chunks_in_memory);
    BOOST_TEST_MESSAGE("Number of used chunks: " << used_chunks);
    BOOST_TEST(reachable_chunks_by_allocator == total_chunks_in_memory - used_chunks);
}

static void init_heap(size_t mem_size)
{
    auto [start_addr, end_addr] = get_aligned_memory_region(mem_size);
    fill_memory_region_with_random_data(start_addr, end_addr);
    size_t num_bytes = diff(start_addr, end_addr);
    holder::heap((void *)start_addr, num_bytes);
}


BOOST_AUTO_TEST_CASE(allocator_initialize_memory_test)
{
    init_heap(512);
    inblock_allocator<int, holder> allocator;

    auto stats = get_allocator_stats(allocator);
    dump_allocator_stats(stats);
    check_allocator_stats(stats);
    check_allocator_consistency(stats);

    BOOST_TEST_MESSAGE("Checking if memory is filled with chunks");
    check_memory_filled_with_chunks(stats.used_mem_start, stats.used_mem_end);
}

BOOST_AUTO_TEST_CASE(allocator_allocated_data_are_aligned_test)
{
    init_heap(10 * 1024);
    inblock_allocator<uint8_t, holder> allocator;

    auto stats = get_allocator_stats(allocator);
    dump_allocator_stats(stats);
    check_allocator_stats(stats);
    check_allocator_consistency(stats);

    for (size_t payload_size = 23; payload_size < 80; payload_size += 5) {
        uint8_t *data = allocator.allocate(payload_size);
        BOOST_TEST(data);
        BOOST_TEST(is_aligned((intptr_t)data));
    }
}

BOOST_AUTO_TEST_CASE(allocator_consistency_after_small_allocs_test)
{
    init_heap(10 * 1024);
    inblock_allocator<uint8_t, holder> allocator;

    auto stats = get_allocator_stats(allocator);
    dump_allocator_stats(stats);
    check_allocator_stats(stats);
    check_allocator_consistency(stats);

    for (size_t payload_size = 23; payload_size < 80; payload_size += 5) {
        uint8_t *data = allocator.allocate(payload_size);
        BOOST_TEST(data);
    }
    BOOST_TEST_MESSAGE("Checking allocator consistency after some small allocs");
    stats = get_allocator_stats(allocator);
    dump_allocator_stats(stats);
    check_allocator_consistency(stats);
}

BOOST_AUTO_TEST_CASE(allocator_two_phase_small_allocs_consistency_test)
{
    init_heap(10 * 1024);
    inblock_allocator<uint8_t, holder> allocator;

    auto stats = get_allocator_stats(allocator);
    dump_allocator_stats(stats);
    check_allocator_stats(stats);
    check_allocator_consistency(stats);

    BOOST_TEST_MESSAGE("Starting first phase of allocations...");
    std::vector<std::pair<uint8_t *, size_t>> first_phase_allocated_data;
    for (size_t payload_size = 23; payload_size < 80; payload_size += 5) {
        uint8_t *data = allocator.allocate(payload_size);
        BOOST_TEST(data);
        first_phase_allocated_data.emplace_back(data, payload_size);
        fill_payload(data, payload_size);
    }
    BOOST_TEST_MESSAGE("Allocator stats after first phase:");
    stats = get_allocator_stats(allocator);
    dump_allocator_stats(stats);
    check_allocator_consistency(stats);

    BOOST_TEST_MESSAGE("Starting second phase of allocations...");
    std::vector<std::pair<uint8_t *, size_t>> second_phase_allocated_data;
    for (size_t payload_size = 60; payload_size < 150; payload_size += 5) {
        uint8_t *data = allocator.allocate(payload_size);
        BOOST_TEST(data);
        second_phase_allocated_data.emplace_back(data, payload_size);
        fill_payload(data, payload_size);
    }
    BOOST_TEST_MESSAGE("Allocator stats after second phase:");
    stats = get_allocator_stats(allocator);
    dump_allocator_stats(stats);
    check_allocator_consistency(stats);


    BOOST_TEST_MESSAGE("Checking consistency of first phase data...");
    for (auto &&data : first_phase_allocated_data) {
        BOOST_TEST(check_payload_consistency(data.first, data.second));
    }

    BOOST_TEST_MESSAGE("Checking consistency of second phase data...");
    for (auto &&data : second_phase_allocated_data) {
        BOOST_TEST(check_payload_consistency(data.first, data.second));
    }
}

BOOST_AUTO_TEST_CASE(allocator_consolidate_memory_test)
{
    init_heap(10 * 1024);
    inblock_allocator<uint8_t, holder> allocator;

    auto stats = get_allocator_stats(allocator);
    dump_allocator_stats(stats);
    check_allocator_stats(stats);
    check_allocator_consistency(stats);

    size_t data_size = 5 * 1024;
    uint8_t * data = allocator.allocate(data_size);
    BOOST_TEST(data);

    BOOST_TEST_MESSAGE("Checking consistency after big allocation...");
    stats = get_allocator_stats(allocator);
    dump_allocator_stats(stats);
    check_allocator_consistency(stats);
}

BOOST_AUTO_TEST_CASE(allocator_small_allocs_after_big_alloc_test)
{
    init_heap(10 * 1024);
    inblock_allocator<uint8_t, holder> allocator;

    auto stats = get_allocator_stats(allocator);
    dump_allocator_stats(stats);
    check_allocator_stats(stats);
    check_allocator_consistency(stats);

    BOOST_TEST_MESSAGE("Big allocation...");
    uint8_t *big_data = allocator.allocate(5 * 1024);
    BOOST_TEST(big_data);
    BOOST_TEST_MESSAGE("Checking consistency after big allocation...");
    stats = get_allocator_stats(allocator);
    dump_allocator_stats(stats);
    check_allocator_consistency(stats);

    BOOST_TEST_MESSAGE("Small allocations...");
    for (size_t data_size = 46; data_size < 63; data_size += 3) {
        uint8_t *data = allocator.allocate(data_size);
        BOOST_TEST(data);
        BOOST_TEST_MESSAGE("Stats after one small alloc:");
        stats = get_allocator_stats(allocator);
        dump_allocator_stats(stats);
    }

    BOOST_TEST_MESSAGE("Checking consistency after small allocations...");
    stats = get_allocator_stats(allocator);
    dump_allocator_stats(stats);
    check_allocator_consistency(stats);
}

BOOST_AUTO_TEST_CASE(allocator_random_peek_alloc_test)
{
    const size_t heap_size = 20 * 1024;
    const double peek = 0.6;
    const size_t peek_heap_size = (size_t)((double)heap_size * peek);
    const size_t max_data_size = 2 * 1024;
    int remaining_mem = peek_heap_size;

    init_heap(heap_size);
    inblock_allocator<uint8_t, holder> allocator;
    BOOST_TEST_MESSAGE("Total memory size = " << heap_size);
    BOOST_TEST_MESSAGE("Peek memory size = " << peek_heap_size);
    auto stats = get_allocator_stats(allocator);
    dump_allocator_stats(stats);
    check_allocator_stats(stats);
    check_allocator_consistency(stats);

    while (remaining_mem > 0) {
        size_t data_size = rand() % max_data_size;
        BOOST_TEST_MESSAGE("Next data size to allocate = " << data_size);

        uint8_t *data = allocator.allocate(data_size);
        BOOST_TEST(data);

        BOOST_TEST_MESSAGE("\t\t Stats after one allocation of size = " << data_size);
        stats = get_allocator_stats(allocator);
        dump_allocator_stats(stats);
        check_allocator_consistency(stats);

        remaining_mem -= data_size;
    }

    BOOST_TEST_MESSAGE("Checking consistency after peek allocations...");
    stats = get_allocator_stats(allocator);
    dump_allocator_stats(stats);
    check_allocator_consistency(stats);
}

BOOST_AUTO_TEST_CASE(allocator_simple_dealloc_test)
{
    init_heap(2 * 1024);
    inblock_allocator<uint8_t, holder> allocator;
    auto stats = get_allocator_stats(allocator);

    size_t data_size = 48;
    uint8_t *data = allocator.allocate(data_size);
    allocator.deallocate(data, data_size);

    BOOST_TEST_MESSAGE("Stats after deallocation:");
    stats = get_allocator_stats(allocator);
    dump_allocator_stats(stats);

    traverse_all_memory(stats.used_mem_start, stats.used_mem_end, [](chunk_t *chunk) {
        BOOST_TEST(!chunk->used);
    });
}

BOOST_AUTO_TEST_CASE(allocator_alloc_and_dealloc_random_test)
{
    init_heap(20 * 1024);
    inblock_allocator<uint8_t, holder> allocator;
    auto stats = get_allocator_stats(allocator);
    dump_allocator_stats(stats);

    BOOST_TEST(count_used_chunks(stats.used_mem_start, stats.used_mem_end) == 0);

    const size_t iterations = 300;
    const size_t max_data_size = 512;
    BOOST_TEST_MESSAGE("Max data size = " << max_data_size);

    std::vector<std::pair<uint8_t *, size_t>> allocated_data;
    for (size_t i = 0; i < 300; i++) {
        if (rand() % 2 == 0) {
            size_t data_size = (rand() % max_data_size) + 1;
            uint8_t *data = allocator.allocate(data_size);
            BOOST_TEST(data);
            allocated_data.emplace_back(data, data_size);
        }
        else {
            if (allocated_data.empty()) {
                continue;
            }
            size_t data_idx = rand() % allocated_data.size();
            auto data = allocated_data[data_idx];
            allocator.deallocate(data.first, data.second);

            allocated_data.erase(allocated_data.begin() + data_idx);
        }

        size_t used_chunks = count_used_chunks(stats.used_mem_start, stats.used_mem_end);
        BOOST_TEST(used_chunks == allocated_data.size());
    }

    BOOST_TEST_MESSAGE("Checking consistency after alloc/dealloc...");
    stats = get_allocator_stats(allocator);
    dump_allocator_stats(stats);
    check_allocator_consistency(stats);
}

template<typename V>
using Vector = std::vector<V, inblock_allocator<V, holder>>;

BOOST_AUTO_TEST_CASE(allocator_in_std_container_mem_init_test)
{
    std::vector<uint8_t> mem;
    mem.resize(2500000);

    holder::heap(mem.data(), 2500000);
    Vector<int> v;

    auto stats = get_allocator_stats(v.get_allocator());
    dump_allocator_stats(stats);
    check_allocator_stats(stats);
    check_allocator_consistency(stats);

    // Make small allocation
    v.push_back(42);

    BOOST_TEST_MESSAGE("Stats after small alloc:");
    stats = get_allocator_stats(v.get_allocator());
    dump_allocator_stats(stats);
    check_allocator_consistency(stats);
}
