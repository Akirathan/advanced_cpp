#include "inblock_allocator.hpp"

address_t inblock_allocator_heap::start_addr = 0;
address_t inblock_allocator_heap::end_addr = 0;
size_t inblock_allocator_heap::size = 0;
