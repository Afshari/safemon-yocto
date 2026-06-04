cmake_minimum_required(VERSION 3.16)
project(safemon_libs VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

add_subdirectory(ecdsa)
# add_subdirectory(grpc)
# add_subdirectory(redis)