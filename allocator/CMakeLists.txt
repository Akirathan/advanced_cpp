cmake_minimum_required(VERSION 3.13)

add_compile_options("-Wall" "-Wextra")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g ")

add_compile_definitions("BOOST_ALL_DYN_LINK")
find_package(Boost COMPONENTS log system unit_test_framework REQUIRED)

set(SOURCES
        allocator_exception.hpp
        common.hpp
        chunk.hpp
        inblock_allocator.hpp
        inblock_allocator.cpp
        tests/test_common.hpp
        )

# My main
add_executable(allocator
        ${SOURCES}
        main.cpp
        )

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


include(unit_tests.cmake)

