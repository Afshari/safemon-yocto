add_library(input
    src/evdev_keyboard.cpp
    src/evdev_mouse.cpp
    src/input_manager.cpp
)

target_include_directories(input
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    enable_testing()
    add_subdirectory(test input_test)
endif()