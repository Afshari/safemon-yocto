add_library(can_bridge
    src/can_reader.cpp
    src/can_bridge.cpp
)

target_include_directories(can_bridge
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_link_libraries(can_bridge
    PUBLIC common
)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    enable_testing()
    add_subdirectory(test can_bridge_test)
endif()