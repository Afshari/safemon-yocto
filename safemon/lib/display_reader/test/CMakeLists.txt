find_package(GTest REQUIRED)

add_executable(display_reader_tests
    test_display_reader.cpp
)

target_link_libraries(display_reader_tests
    PRIVATE
        display_reader
        GTest::GTest
        GTest::Main
        GTest::gmock
)

include(GoogleTest)
gtest_discover_tests(display_reader_tests)