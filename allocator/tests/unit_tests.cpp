
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

struct holder {
    static inblock_allocator_heap heap;
};

inblock_allocator_heap holder::heap;

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

static double count_chunk_headers_overhead(intptr_t start_addr, intptr_t end_addr)
{
    size_t total_memory = diff(start_addr, end_addr);

    size_t chunk_headers_size_sum = 0;
    traverse_all_memory(start_addr, end_addr, [&](chunk_t *chunk) {
        chunk_headers_size_sum += chunk_header_size;
    });

    return ((double)total_memory / (double)chunk_headers_size_sum);
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

static bool is_chunk_in_initialized_state(const chunk_t *chunk)
{
    return chunk->payload_size > 0 && chunk->used == 0;
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
    });
}

BOOST_AUTO_TEST_CASE(small_bins_simple_allocation_test)
{
    SmallBins small_bins = initialize_small_bins(80, nullptr, nullptr);

    size_t chunk_payload_size = SmallBins::min_chunk_size_for_bins;
    BOOST_TEST(small_bins.contains_bin_with_chunk_size(chunk_payload_size));
    chunk_t *allocated_chunk = small_bins.allocate_chunk(chunk_payload_size);
    BOOST_TEST(allocated_chunk);
    BOOST_TEST(allocated_chunk->payload_size == chunk_payload_size);
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


BOOST_AUTO_TEST_CASE(small_bins_alloc_all_memory_test)
{
    intptr_t start_addr = 0;
    intptr_t end_addr = 0;
    SmallBins small_bins = initialize_small_bins(1024 * 1024, &start_addr, &end_addr);

    size_t chunks_count_before_alloc = small_bins.get_total_chunks_size();
    BOOST_TEST_MESSAGE("Chunks count before alloc = " << chunks_count_before_alloc);

    // Allocate all chunks from smallest bin, also count how many chunks were allocated.
    size_t smallest_payload_size = SmallBins::min_chunk_size_for_bins;
    chunk_t *allocated_chunk = small_bins.allocate_chunk(smallest_payload_size);
    size_t allocated_chunks_count = 1;
    while (allocated_chunk) {
        allocated_chunk = small_bins.allocate_chunk(smallest_payload_size);
        if (allocated_chunk) {
            allocated_chunks_count++;
        }
    }
    BOOST_TEST(small_bins.get_total_chunks_size() == 0);
    BOOST_TEST_MESSAGE("Allocated chunks count = " << allocated_chunks_count);
    BOOST_TEST(allocated_chunks_count >= chunks_count_before_alloc);

    size_t should_test = 492;
    traverse_all_memory(start_addr, end_addr, [&](chunk_t *chunk) {
        if (should_test == 0) {
            BOOST_TEST(chunk->payload_size >= smallest_payload_size);
            should_test = 492;
        }
        should_test--;
    });
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
    });
}