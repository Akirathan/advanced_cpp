project(unit_tests CXX)

find_package(Boost COMPONENTS unit_test_framework REQUIRED)

include_directories(/usr/include/boost/test)


add_executable(unit_tests
        ${SOURCES}
        tests/unit_tests.cpp
        )

target_link_libraries(unit_tests ${Booost_UNIT_TEST_FRAMEWORK_LIBRARY})
