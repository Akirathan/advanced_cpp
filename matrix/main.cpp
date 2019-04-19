#include <vector>
#include <iostream>
#include "my_array.hpp"

template <typename T>
static void print_array(my_array<T> &array)
{
    for (auto it = array.begin(); it != array.end(); ++it) {
        std::cout << *it << std::endl;
    }
}

int main()
{
    std::vector<int> vec = {1, 2, 3};

    my_array<int> array{vec};

    print_array(array);

    for (auto it = array.begin(); it != array.end(); ++it) {
        *it = 42;
    }

    for (auto &&item : array) {
        std::cout << item << std::endl;
    }
}
