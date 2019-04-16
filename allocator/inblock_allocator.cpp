#include "inblock_allocator.hpp"

chunk_list_t inblock_allocator_heap::chunk_list = nullptr;
address_t inblock_allocator_heap::start_addr = 0;
address_t inblock_allocator_heap::end_addr = 0;
size_t inblock_allocator_heap::size = 0;
size_t inblock_allocator_heap::allocators_count = 0;
