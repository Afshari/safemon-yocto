cmake_minimum_required(VERSION 3.16)
project(safemon VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_library(HIREDIS_LIB hiredis REQUIRED)
find_path(HIREDIS_INCLUDE hiredis/hiredis.h REQUIRED)

add_executable(safemon-app src/main.cpp)

target_include_directories(safemon-app PRIVATE
    include
    ${HIREDIS_INCLUDE}
)

target_link_libraries(safemon-app PRIVATE
    ${HIREDIS_LIB}
)

install(TARGETS safemon-app DESTINATION bin)