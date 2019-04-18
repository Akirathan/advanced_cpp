project(matrix)
cmake_minimum_required(VERSION 3.13)

add_compile_options("-Wall" "-Wextra")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g ")

add_compile_definitions("BOOST_ALL_DYN_LINK")
find_package(Boost COMPONENTS log system unit_test_framework REQUIRED)

set(SOURCES
        matrix.hpp
        )

add_executable(matrix
        ${SOURCES}
        main.cpp
        )

target_link_libraries(matrix ${Boost_LIBRARIES})

include(unit_tests.cmake)

