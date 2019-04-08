
#define BOOST_TEST_MODULE My_test
#include <boost/test/included/unit_test.hpp>
#include "../inblock_allocator.hpp"

static constexpr uint8_t MAGIC = 0xA3;

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

BOOST_AUTO_TEST_CASE(sample_test)
{
    const int value = 3;

    BOOST_TEST(value == 3);
}

BOOST_AUTO_TEST_CASE(two_chunks)
{
    std::array<uint8_t, 100> mem = {};
    const size_t payload_size = 16;
    intptr_t start_addr = reinterpret_cast<intptr_t>(mem.data());
    chunk_t *first_chunk = initialize_chunk(start_addr, payload_size);
    fill_payload(get_chunk_data(first_chunk), payload_size);

    intptr_t next_addr = start_addr + get_chunk_size(first_chunk);
    chunk_t *second_chunk = initialize_chunk(next_addr, payload_size);
    fill_payload(get_chunk_data(second_chunk), payload_size);

    // Check payload of both chunks
    BOOST_TEST(check_payload_consistency(get_chunk_data(first_chunk), payload_size));
    BOOST_TEST(check_payload_consistency(get_chunk_data(second_chunk), payload_size));


}

