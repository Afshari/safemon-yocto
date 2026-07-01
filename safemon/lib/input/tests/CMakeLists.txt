find_package(GTest REQUIRED)
find_package(Threads REQUIRED)

add_executable(input_test
    test_input_event_types.cpp
    test_evdev_keyboard.cpp
    test_evdev_mouse.cpp
)

target_link_libraries(input_test
    PRIVATE
        input
        GTest::gtest
        GTest::gtest_main
        Threads::Threads
)

include(GoogleTest)
gtest_discover_tests(input_test)