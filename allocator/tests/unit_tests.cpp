
#define BOOST_TEST_MODULE My_test

#include <array>
#include <boost/test/included/unit_test.hpp>
#include "../inblock_allocator.hpp"

static constexpr uint8_t MAGIC = 0xA3;

struct holder {
    static inblock_allocator_heap heap;
};

inblock_allocator_heap holder::heap;

static constexpr size_t memory_size = 5 * 1024;
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

BOOST_AUTO_TEST_CASE(chunk_split_test)
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

BOOST_AUTO_TEST_CASE(aligned_heap_test)
{
    std::array<uint8_t, 100> mem = {};
    inblock_allocator_heap heap;
    heap((void *)mem.data(), 100);

    BOOST_TEST(is_aligned(heap.get_start_addr()));
    BOOST_TEST(is_aligned(heap.get_end_addr()));
    BOOST_TEST(heap.get_size() <= 100);
}

BOOST_AUTO_TEST_CASE(small_bins_memory_initialization)
{
    auto [start_addr, end_addr] = get_aligned_memory_region(1024);
    fill_memory_region_with_random_data(start_addr, end_addr);

    SmallBins<uint8_t, holder> small_bins;
    intptr_t returned_addr = small_bins.initialize_memory(start_addr, end_addr);

    BOOST_TEST(returned_addr <= end_addr);

    // Traverse memory from start_addr to returned_addr and check whether it contains chunks.
    chunk_t *chunk = reinterpret_cast<chunk_t *>(start_addr);
    while (start_addr != returned_addr) {
        start_addr += get_chunk_size(chunk);
        BOOST_TEST(is_chunk_in_initialized_state(chunk));
        chunk = next_chunk_in_mem(chunk);
    }
}

