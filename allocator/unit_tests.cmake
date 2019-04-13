project(unit_tests CXX)

include_directories(${Boost_INCLUDE_DIR})

add_executable(unit_tests
        ${SOURCES}
        tests/unit_tests.cpp
        )

target_link_libraries(unit_tests ${Boost_LIBRARIES})
