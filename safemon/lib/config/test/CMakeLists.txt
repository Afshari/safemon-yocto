find_package(GTest REQUIRED)

add_executable(config_tests
    test_config.cpp
)

target_link_libraries(config_tests
    PRIVATE
        config
        GTest::GTest
        GTest::Main
)

include(GoogleTest)
gtest_discover_tests(config_tests)