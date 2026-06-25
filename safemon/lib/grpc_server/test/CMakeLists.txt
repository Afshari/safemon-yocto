find_package(GTest REQUIRED)

add_executable(grpc_server_tests
    test_grpc_server.cpp
)

target_link_libraries(grpc_server_tests
    PRIVATE
        grpc_server
        GTest::GTest
        GTest::Main
        GTest::gmock
)

include(GoogleTest)
gtest_discover_tests(grpc_server_tests)