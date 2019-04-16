#ifndef COMMON_HPP
#define COMMON_HPP

#include <cstddef>
#include <cassert>
#include <cmath>
#include <cstdint>

constexpr size_t alignment = 8;
using address_t = uintptr_t;

constexpr size_t align_size_up(size_t size) noexcept
{
    while (size % alignment != 0) {
        size++;
    }
    return size;
}

inline bool is_aligned(address_t ptr)
{
    return ptr % alignment == 0;
}

inline size_t diff(address_t addr_a, address_t addr_b)
{
    if (addr_a <= addr_b) {
        return addr_b - addr_a;
    }
    else {
        return addr_a - addr_b;
    }
}

#endif //COMMON_HPP
