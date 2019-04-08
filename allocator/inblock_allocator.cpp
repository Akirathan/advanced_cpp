#include "inblock_allocator.hpp"

intptr_t inblock_allocator_heap::start_addr = 0;
intptr_t inblock_allocator_heap::end_addr = 0;
size_t inblock_allocator_heap::size = 0;
