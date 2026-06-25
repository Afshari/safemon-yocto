find_package(Protobuf REQUIRED)
find_package(gRPC CONFIG REQUIRED)

add_library(grpc_server
    src/grpc_server.cpp
    src/fault_event_builder.cpp
)

target_include_directories(grpc_server
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/inc
    PRIVATE ${Protobuf_INCLUDE_DIRS}
)

target_link_libraries(grpc_server
    PUBLIC  common config
    PRIVATE protobuf::libprotobuf
            gRPC::grpc++
)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    enable_testing()
    add_subdirectory(test grpc_server_test)
endif()