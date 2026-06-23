find_package(GTest REQUIRED)

add_executable(fault_detector_tests
    test_fault_rules.cpp
    test_fault_detector.cpp
)

target_link_libraries(fault_detector_tests
    PRIVATE
        fault_detector
        GTest::GTest
        GTest::Main
        GTest::gmock
)

include(GoogleTest)
gtest_discover_tests(fault_detector_tests)