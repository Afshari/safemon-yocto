add_library(config
    src/config.cpp
)

target_include_directories(config
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    enable_testing()
    add_subdirectory(test config_test)
endif()