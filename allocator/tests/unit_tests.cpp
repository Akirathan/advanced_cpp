
#define BOOST_TEST_MODULE My_test

#include <array>
#include <memory>
#include <functional>
#include <boost/test/included/unit_test.hpp>
#include <ostream>
#include "../inblock_allocator.hpp"
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

static std::vector<std::unique_ptr<chunk_t>> create_chunks(size_t count)
{
    std::vector<std::unique_ptr<chunk_t>> chunk_vector;
    for (size_t i = 0; i < count; ++i) {
        chunk_vector.emplace_back(std::make_unique<chunk_t>());
    }
    return chunk_vector;
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

/* ===================================================================================================== */
/* ============================== HEAP TESTS ===================================================== */
/* ===================================================================================================== */


/* ===================================================================================================== */
/* ============================== ALLOCATOR TESTS ===================================================== */
/* ===================================================================================================== */

struct holder {
    static inblock_allocator_heap heap;
};

inblock_allocator_heap holder::heap;

struct allocator_stats_t {
    size_t available_mem_size;
    size_t used_mem_size;
    size_t used_chunks;
    size_t used_mem_size_with_chunk_headers;
};

std::ostream &operator<<(std::ostream &os, const allocator_stats_t &stats)
{
    os << "available_mem_size: " << stats.available_mem_size << " used_mem_size: " << stats.used_mem_size
       << " used_chunks: " << stats.used_chunks;
    return os;
}

bool operator==(const allocator_stats_t &stats_1, const allocator_stats_t &stats_2)
{
    return stats_1.used_mem_size == stats_2.used_mem_size && stats_1.used_chunks == stats_2.used_chunks;
}

template <typename T, typename HeapHolder>
allocator_stats_t get_allocator_stats(const inblock_allocator<T, HeapHolder> &allocator)
{
    allocator_stats_t stats{};
    stats.available_mem_size = diff(inblock_allocator<T, HeapHolder>::heap_type::get_start_addr(),
            inblock_allocator<T, HeapHolder>::heap_type::get_end_addr());

    const chunk_t *chunk_list = allocator.get_chunk_list();

    stats.used_mem_size = 0;
    stats.used_chunks = 0;
    stats.used_mem_size_with_chunk_headers = 0;
    const chunk_t *chunk = chunk_list;
    while (chunk) {
        stats.used_chunks++;
        stats.used_mem_size += chunk->payload_size;
        stats.used_mem_size_with_chunk_headers += chunk_header_size + chunk->payload_size;
        chunk = chunk->next;
    }

    return stats;
}

double count_chunk_headers_overhead(const allocator_stats_t &stats)
{
    size_t chunk_header_size_sum = stats.used_chunks * chunk_header_size;
    return (double)chunk_header_size_sum / (double)stats.available_mem_size;
}

void dump_allocator_stats(const allocator_stats_t &stats)
{
    BOOST_TEST_MESSAGE("Available mem size = " << stats.available_mem_size);
    BOOST_TEST_MESSAGE("Used mem size = " << stats.used_mem_size);
    BOOST_TEST_MESSAGE("Used mem size with chunk headers = " << stats.used_mem_size_with_chunk_headers);
    BOOST_TEST_MESSAGE("Chunk headers overhead = " << count_chunk_headers_overhead(stats));
    BOOST_TEST_MESSAGE("Used chunks = " << stats.used_chunks);
}

static void init_heap(size_t mem_size)
{
    auto [start_addr, end_addr] = get_aligned_memory_region(mem_size);
    fill_memory_region_with_random_data(start_addr, end_addr);
    size_t num_bytes = diff(start_addr, end_addr);
    holder::heap((void *)start_addr, num_bytes);
}

BOOST_AUTO_TEST_CASE(allocator_allocated_data_are_aligned_test)
{
    init_heap(10 * 1024);
    inblock_allocator<uint8_t, holder> allocator;

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

    size_t allocated_chunks = 0;
    size_t allocated_size = 0;
    for (size_t payload_size = 23; payload_size < 80; payload_size += 5) {
        uint8_t *data = allocator.allocate(payload_size);
        BOOST_TEST(data);
        allocated_chunks++;
        allocated_size += payload_size;
    }

    BOOST_TEST_MESSAGE("Checking allocator consistency after some small allocs");
    auto stats = get_allocator_stats(allocator);
    dump_allocator_stats(stats);
    BOOST_TEST(stats.used_mem_size >= allocated_size);
    BOOST_TEST(stats.used_chunks == allocated_chunks);
}

BOOST_AUTO_TEST_CASE(allocator_random_peek_alloc_test)
{
    const size_t heap_size = 10 * 1024;
    const double peek = 0.6;
    const size_t peek_heap_size = (size_t)((double)heap_size * peek);
    const size_t max_data_size = 2 * 1024;
    int remaining_mem = peek_heap_size;

    init_heap(heap_size);
    inblock_allocator<uint8_t, holder> allocator;
    BOOST_TEST_MESSAGE("Total memory size = " << heap_size);
    BOOST_TEST_MESSAGE("Peek memory size = " << peek_heap_size);

    while (remaining_mem > 0) {
        size_t data_size = rand() % max_data_size;
        BOOST_TEST_MESSAGE("Next data size to allocate = " << data_size);

        uint8_t *data = allocator.allocate(data_size);
        BOOST_TEST(data);

        BOOST_TEST_MESSAGE("\t\t Stats after one allocation of size = " << data_size);
        auto stats = get_allocator_stats(allocator);
        dump_allocator_stats(stats);

        remaining_mem -= data_size;
    }

    BOOST_TEST_MESSAGE("Stats after peek allocations...");
    auto stats = get_allocator_stats(allocator);
    dump_allocator_stats(stats);
}

BOOST_AUTO_TEST_CASE(allocator_simple_dealloc_test)
{
    init_heap(2 * 1024);
    inblock_allocator<uint8_t, holder> allocator;

    size_t data_size = 48;
    uint8_t *data = allocator.allocate(data_size);
    allocator.deallocate(data, data_size);

    BOOST_TEST_MESSAGE("Stats after deallocation:");
    auto stats = get_allocator_stats(allocator);
    dump_allocator_stats(stats);
    BOOST_TEST(stats.used_chunks == 0);
    BOOST_TEST(stats.used_mem_size == 0);
}

BOOST_AUTO_TEST_CASE(allocator_more_deallocs)
{
    init_heap(5 * 1024);
    inblock_allocator<int, holder> allocator;

    std::vector<std::pair<int *, size_t>> allocated_data;
    for (size_t data_count = 45; data_count < 100; data_count += 5) {
        int* data = allocator.allocate(data_count);
        allocated_data.emplace_back(std::make_pair(data, data_count));
    }

    for (auto &&allocated_item : allocated_data) {
        allocator.deallocate(allocated_item.first, allocated_item.second);
    }
    auto stats = get_allocator_stats(allocator);
    BOOST_TEST(stats.used_chunks == 0);
    BOOST_TEST(stats.used_mem_size == 0);
}

template<typename V>
using Vector = std::vector<V, inblock_allocator<V, holder>>;

BOOST_AUTO_TEST_CASE(allocator_in_vector_one_push_back)
{
    std::vector<uint8_t> mem;
    mem.resize(1024);

    holder::heap(mem.data(), 1024);
    Vector<int> v;

    // Make small allocation
    v.push_back(42);

    BOOST_TEST_MESSAGE("Stats after small alloc:");
    auto stats = get_allocator_stats(v.get_allocator());
    dump_allocator_stats(stats);
    BOOST_TEST(stats.used_mem_size > 0);
}

BOOST_AUTO_TEST_CASE(allocator_in_vector_more_push_backs)
{
    const size_t mem_size = 5 * 1024;
    std::vector<uint8_t> mem;
    mem.resize(mem_size);
    holder::heap(mem.data(), mem_size);

    Vector<int> vec;
    for (size_t i = 0; i < 42; i++) {
        vec.push_back(i);
    }

    BOOST_TEST_MESSAGE("Stats after pushbacks to vector:");
    auto stats = get_allocator_stats(vec.get_allocator());
    dump_allocator_stats(stats);
    BOOST_TEST(stats.used_mem_size > 0);
}

/**
 * Two allocators are initialized. Second allocator is initialized after first one makes
 * some allocations.
 */
BOOST_AUTO_TEST_CASE(second_allocator_initialized_after_first_allocates)
{
    const size_t mem_size = 10 * 1024;
    init_heap(mem_size);

    inblock_allocator<int, holder> allocator_1;
    allocator_1.allocate(10);

    inblock_allocator<int, holder> allocator_2;
    auto stats_2 = get_allocator_stats(allocator_2);
    BOOST_TEST_MESSAGE("Allocator stats 2:");
    dump_allocator_stats(stats_2);
    BOOST_TEST(stats_2.used_mem_size > 0);
    BOOST_TEST(stats_2.used_chunks > 0);
}

BOOST_AUTO_TEST_CASE(second_allocator_deallocates_after_first_one)
{
    init_heap(10 * 1024);
    inblock_allocator<int, holder> allocator_1;
    inblock_allocator<int, holder> allocator_2;

    int *allocation = allocator_1.allocate(45);
    allocator_2.deallocate(allocation, 45);

    auto stats_1 = get_allocator_stats(allocator_1);
    auto stats_2 = get_allocator_stats(allocator_2);
    dump_allocator_stats(stats_1);
    dump_allocator_stats(stats_2);
    BOOST_TEST(stats_1 == stats_2);
}

BOOST_AUTO_TEST_CASE(allocate_in_more_iterations)
{
    const size_t mem_size = 3 * 1024 * 1024;
    std::vector<uint8_t> mem;
    mem.resize(mem_size);
    holder::heap(mem.data(), mem_size);

    const size_t repetitions = 3;
    const size_t pushbacks = 100 * 1000;
    size_t check_time = 426;

    for (size_t i = 0; i < repetitions; i++) {
        Vector<int> vec;

        for (size_t j = 0; j < pushbacks; j++) {
            vec.push_back(j);
            if (check_time == 0) {
                auto stats = get_allocator_stats(vec.get_allocator());
                dump_allocator_stats(stats);
                BOOST_TEST(stats.used_chunks > 0);
                BOOST_TEST(stats.used_mem_size > 0);
                check_time = 426;
            }
            check_time--;
        }
    }
}