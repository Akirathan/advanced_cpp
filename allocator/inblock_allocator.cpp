#include "inblock_allocator.hpp"

address_t inblock_allocator_heap::chunk_region_start_addr = 0;
address_t inblock_allocator_heap::chunk_region_end_addr = 0;
SmallBins inblock_allocator_heap::small_bins{};
LargeBin inblock_allocator_heap::large_bin{};
address_t inblock_allocator_heap::start_addr = 0;
address_t inblock_allocator_heap::end_addr = 0;
size_t inblock_allocator_heap::size = 0;
size_t inblock_allocator_heap::allocators_count = 0;
