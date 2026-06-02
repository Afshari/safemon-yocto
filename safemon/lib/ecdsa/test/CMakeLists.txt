find_package(GTest REQUIRED)

add_executable(ecdsa_tests
    test_bigint.cpp
    test_ec_point.cpp
    test_ecdsa.cpp
)

target_link_libraries(ecdsa_tests
    PRIVATE
        ecdsa
        GTest::GTest
        GTest::Main
)

include(GoogleTest)
gtest_discover_tests(ecdsa_tests)