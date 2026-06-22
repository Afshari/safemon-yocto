find_package(GTest REQUIRED)

add_executable(can_bridge_tests
    test_can_bridge.cpp
)

target_link_libraries(can_bridge_tests
    PRIVATE
        can_bridge
        GTest::GTest
        GTest::Main
        GTest::gmock
)

include(GoogleTest)
gtest_discover_tests(can_bridge_tests)