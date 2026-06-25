find_package(GTest REQUIRED)

add_executable(grpc_server_tests
    test_grpc_server.cpp
)

target_link_libraries(grpc_server_tests
    PRIVATE
        fault_event_builder
        GTest::GTest
        GTest::Main
        GTest::gmock
)

include(GoogleTest)
gtest_discover_tests(grpc_server_tests)