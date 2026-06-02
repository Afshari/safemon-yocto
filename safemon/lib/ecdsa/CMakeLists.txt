cmake_minimum_required(VERSION 3.16)
project(ecdsa VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# GMP
find_library(GMP_LIB gmp REQUIRED)
find_path(GMP_INCLUDE gmp.h REQUIRED)

# ECDSA library
add_library(ecdsa
    src/bigint.cpp
    src/ec_point.cpp
    src/ecdsa.cpp
)

target_include_directories(ecdsa
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/inc
    PRIVATE ${GMP_INCLUDE}
)

target_link_libraries(ecdsa PRIVATE ${GMP_LIB})

# Tests (only when built directly, not when included from parent)
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    enable_testing()
    add_subdirectory(test)
endif()