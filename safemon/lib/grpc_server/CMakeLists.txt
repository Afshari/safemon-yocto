add_library(fault_event_builder
    src/fault_event_builder.cpp
)

target_include_directories(fault_event_builder
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_link_libraries(fault_event_builder
    PUBLIC common config
)

if(NOT CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    find_package(Protobuf REQUIRED)
    find_package(gRPC CONFIG REQUIRED)

    add_library(grpc_server
        src/grpc_server.cpp
    )

    target_include_directories(grpc_server
        PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/inc
        PRIVATE ${Protobuf_INCLUDE_DIRS}
    )

    target_link_libraries(grpc_server
        PUBLIC  common config fault_event_builder
        PRIVATE protobuf::libprotobuf
                gRPC::grpc++
    )
endif()

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    enable_testing()
    add_subdirectory(test grpc_server_test)
endif()