#include <cstddef>

class inblock_allocator_heap {
    // ...your static data here...
    void operator()(void *ptr, size_t n_bytes) {

    };
};

template<typename T, typename HeapHolder>
class inblock_allocator {
    // ...your solution here...
};
