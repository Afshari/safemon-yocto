find_library(HIREDIS_LIB hiredis REQUIRED)
find_path(HIREDIS_INCLUDE hiredis/hiredis.h REQUIRED)

add_library(fault_detector
    src/fault_rules.cpp
    src/fault_detector.cpp
    src/redis_client_impl.cpp
)

target_include_directories(fault_detector
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc
    PRIVATE ${HIREDIS_INCLUDE}
)

target_link_libraries(fault_detector
    PUBLIC config common
    PRIVATE ${HIREDIS_LIB}
)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    enable_testing()
    add_subdirectory(test fault_detector_test)
endif()