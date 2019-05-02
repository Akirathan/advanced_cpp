project(concurent_bitmap_unit_tests CXX)

include_directories(${Boost_INCLUDE_DIR})

add_executable(unit_tests
        ${SOURCES}
        unit_tests.cpp
        )

target_link_libraries(unit_tests ${Boost_LIBRARIES})


add_executable(du3test
        ${SOURCES}
        du3test.cpp)

target_link_libraries(du3test ${Boost_LIBRARIES})
