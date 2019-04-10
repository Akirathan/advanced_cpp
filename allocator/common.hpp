#ifndef COMMON_HPP
#define COMMON_HPP

#include <cstddef>
#include <cassert>
#include <cmath>
#include <cstdint>

constexpr size_t alignment = 8;

constexpr size_t align_size(size_t size, size_t alignment) noexcept
{
    size += size % alignment;
    return size;
}

inline bool is_aligned(intptr_t ptr)
{
    return ptr % alignment == 0;
}

inline size_t diff(intptr_t ptr, intptr_t intptr)
{
    return std::abs(ptr - intptr);
}

#endif //COMMON_HPP