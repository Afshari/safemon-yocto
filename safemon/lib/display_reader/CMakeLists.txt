add_library(display_reader
    src/display_reader.cpp
)

target_include_directories(display_reader
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_link_libraries(display_reader
    PUBLIC common
)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    enable_testing()
    add_subdirectory(test display_reader_test)
endif()