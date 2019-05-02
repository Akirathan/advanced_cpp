project(unit_tests CXX)

include_directories(${Boost_INCLUDE_DIR})

add_executable(unit_tests
        ${SOURCES}
        tests/test_matrix.cpp
        )

target_link_libraries(unit_tests ${Boost_LIBRARIES})


add_executable(du2test
        ${SOURCES}
        tests/du2test.cpp)

target_link_libraries(du2test ${Boost_LIBRARIES})
