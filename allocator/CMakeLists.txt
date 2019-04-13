cmake_minimum_required(VERSION 3.13)

set(CMAKE_CXX_STANDARD 17)
add_compile_options("-Wall" "-Wextra")
add_compile_definitions("BOOST_ALL_DYN_LINK")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -Og")

find_package(Boost COMPONENTS log system unit_test_framework REQUIRED)
#find_package(Threads)

include_directories(${Boost_INCLUDE_DIR})


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

# Link with boost libs
target_link_libraries(allocator ${Boost_LIBRARIES})


# Basic test
add_executable(basic_test
        ${SOURCES}
        tests/basic_test.cpp
        )

target_link_libraries(basic_test ${Boost_LIBRARIES})

# Matrix test
add_executable(matrix_test
        ${SOURCES}
        tests/matrix_test.cpp
        )

target_link_libraries(matrix_test ${Boost_LIBRARIES})

# Include unit tests project
include(unit_tests.cmake)
