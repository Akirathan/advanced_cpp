cmake_minimum_required(VERSION 3.13)

add_compile_options("-Wall" "-Wextra")


# My main
add_executable(allocator
        allocator_exception.hpp
        inblock_allocator.hpp
        inblock_allocator.cpp
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

# Include unit tests project
include(unit_tests.cmake)
