cmake_minimum_required(VERSION 3.13)

set(CMAKE_CXX_STANDARD 17)
add_compile_options("-Wall" "-Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")


set(SOURCES
        common.hpp
        allocator_exception.hpp
        chunk.hpp
        chunk_list.hpp
        small_bins.hpp
        large_bin.hpp
        inblock_allocator.hpp
        inblock_allocator.cpp
        )

# My main
add_executable(allocator
        ${SOURCES}
        main.cpp
        )


# Basic test
add_executable(basic_test
        ${SOURCES}
        tests/basic_test.cpp
        )

# Matrix test
add_executable(matrix_test
        ${SOURCES}
        tests/matrix_test.cpp
        )

# Include unit tests project
include(unit_tests.cmake)
