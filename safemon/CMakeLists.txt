cmake_minimum_required(VERSION 3.16)
project(safemon VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(HIREDIS REQUIRED hiredis)

add_executable(safemon src/main.cpp)

target_include_directories(safemon PRIVATE
    include
    ${HIREDIS_INCLUDE_DIRS}
)

target_link_libraries(safemon PRIVATE
    ${HIREDIS_LIBRARIES}
)

install(TARGETS safemon DESTINATION bin)