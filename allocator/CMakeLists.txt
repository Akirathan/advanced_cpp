cmake_minimum_required(VERSION 3.13)

add_compile_options("-Wall" "-Wextra")


# My main
add_executable(allocator
        inblock_allocator.hpp
        main.cpp
        )


# Basic test
add_executable(basic_test
        inblock_allocator.hpp
        tests/basic_test.cpp
        )

# Matrix test
add_executable(matrix_test
        inblock_allocator.hpp
        tests/matrix_test.cpp
        )

