cmake_minimum_required(VERSION 3.16)
project(safemon VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#  Dependencies
find_library(HIREDIS_LIB hiredis REQUIRED)
find_path(HIREDIS_INCLUDE hiredis/hiredis.h REQUIRED)

find_library(GMP_LIB gmp REQUIRED)
find_path(GMP_INCLUDE gmp.h REQUIRED)
find_package(OpenSSL REQUIRED)

find_package(Protobuf REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(absl REQUIRED)

find_library(DRM_LIB drm REQUIRED)
find_path(DRM_INCLUDE xf86drm.h REQUIRED)

find_library(EGL_LIB EGL REQUIRED)
find_library(GLES2_LIB GLESv2 REQUIRED)

# ecdsa sources
set(ECDSA_SRC
    lib/ecdsa/src/bigint.cpp
    lib/ecdsa/src/ec_point.cpp
    lib/ecdsa/src/ecdsa.cpp
    lib/ecdsa/src/ecdsa_verify_file.cpp
)

# proto generated sources
set(PROTO_SRC
    proto/fault.pb.cc
    proto/fault.grpc.pb.cc
)

# safemon-app
add_executable(safemon-app
    src/can_reader.cpp
    src/main.cpp
    src/fault_detector.cpp
    src/config.cpp
    src/grpc_server.cpp
    ${ECDSA_SRC}
    ${PROTO_SRC}
)

target_include_directories(safemon-app PRIVATE
    inc
    ${HIREDIS_INCLUDE}
    ${GMP_INCLUDE}
    ${Protobuf_INCLUDE_DIRS}
    lib/ecdsa/inc
    proto
)

target_link_libraries(safemon-app PRIVATE
    ${HIREDIS_LIB}
    ${GMP_LIB}
    OpenSSL::Crypto
    protobuf::libprotobuf
    gRPC::grpc++
    absl::log_internal_check_op
    absl::log_initialize
    absl::log_globals
    absl::log_entry
    absl::log_sink
    absl::log_internal_message
    absl::log_internal_format
    absl::log_internal_globals
    absl::log_internal_log_sink_set
    absl::log_internal_conditions
    absl::log_internal_proto
    absl::log_internal_nullguard
    absl::strings
    absl::base
    absl::raw_logging_internal
    absl::spinlock_wait
    absl::synchronization
    absl::status
    absl::statusor
    absl::time
    absl::time_zone
)

# drm-display
add_executable(drm-display
    src/drm_display.cpp
    src/framebuffer.cpp
)

target_include_directories(drm-display PRIVATE
    inc
    ${HIREDIS_INCLUDE}
    ${DRM_INCLUDE}
    ${DRM_INCLUDE}/drm
)

target_link_libraries(drm-display PRIVATE
    ${DRM_LIB}
    ${HIREDIS_LIB}
)

# Platform selection
set(PLATFORM "rpi4" CACHE STRING "Target platform: rpi4 or jetson")
message(STATUS "Building for platform: ${PLATFORM}")

if(PLATFORM STREQUAL "jetson")
    add_compile_definitions(PLATFORM_JETSON)

    find_library(WAYLAND_CLIENT_LIB wayland-client REQUIRED)
    find_library(WAYLAND_EGL_LIB wayland-egl REQUIRED)
    find_path(WAYLAND_INCLUDE wayland-client.h REQUIRED)

    # Find wayland-scanner
    find_program(WAYLAND_SCANNER wayland-scanner REQUIRED)

    # Find xdg-shell protocol XML
    find_path(WAYLAND_PROTOCOLS_DIR xdg-shell.xml
        PATHS
            /usr/share/wayland-protocols/stable/xdg-shell
            /usr/local/share/wayland-protocols/stable/xdg-shell
        REQUIRED)

    # Generate xdg-shell protocol files
    set(XDG_SHELL_XML "${WAYLAND_PROTOCOLS_DIR}/xdg-shell.xml")
    set(XDG_SHELL_HEADER "${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-client-protocol.h")
    set(XDG_SHELL_SRC    "${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-protocol.c")

    add_custom_command(
        OUTPUT ${XDG_SHELL_HEADER}
        COMMAND ${WAYLAND_SCANNER} client-header ${XDG_SHELL_XML} ${XDG_SHELL_HEADER}
        DEPENDS ${XDG_SHELL_XML}
        COMMENT "Generating xdg-shell client header"
    )
    add_custom_command(
        OUTPUT ${XDG_SHELL_SRC}
        COMMAND ${WAYLAND_SCANNER} private-code ${XDG_SHELL_XML} ${XDG_SHELL_SRC}
        DEPENDS ${XDG_SHELL_XML}
        COMMENT "Generating xdg-shell protocol source"
    )

    # Compile xdg-shell protocol into a small static library
    add_library(xdg_shell_proto STATIC ${XDG_SHELL_SRC})
    target_include_directories(xdg_shell_proto PRIVATE ${WAYLAND_INCLUDE})
    set_target_properties(xdg_shell_proto PROPERTIES LINKER_LANGUAGE C)

    # Ensure header is generated before anything tries to include it
    add_custom_target(xdg_shell_header DEPENDS ${XDG_SHELL_HEADER})
    add_dependencies(xdg_shell_proto xdg_shell_header)

    set(EGL_HELPER_SRC
        src/egl_helper_wayland.cpp
        "${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-protocol.c"
    )
    set(DISPLAY_EXTRA_LIBS ${WAYLAND_CLIENT_LIB} ${WAYLAND_EGL_LIB} ${DRM_LIB} xdg_shell_proto)
    set(DISPLAY_EXTRA_INCLUDES ${WAYLAND_INCLUDE} ${CMAKE_CURRENT_BINARY_DIR})
    set_source_files_properties(
        "${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-protocol.c"
        PROPERTIES GENERATED TRUE
    )
else()
    # RPi4 / Mesa / GBM
    find_library(GBM_LIB gbm REQUIRED)
    find_path(GBM_INCLUDE gbm.h REQUIRED)

    set(EGL_HELPER_SRC src/egl_helper_gbm.cpp)
    set(DISPLAY_EXTRA_LIBS ${GBM_LIB} ${DRM_LIB})
    set(DISPLAY_EXTRA_INCLUDES ${GBM_INCLUDE} ${DRM_INCLUDE} ${DRM_INCLUDE}/drm)
endif()

# egl-triangle
add_executable(egl-triangle
    src/gl_app.cpp
    src/egl_triangle.cpp
    src/drm_helper.cpp
    ${EGL_HELPER_SRC}
    src/config.cpp
)
if(PLATFORM STREQUAL "jetson")
    add_dependencies(egl-triangle xdg_shell_proto)
endif()

target_include_directories(egl-triangle PRIVATE
    inc
    ${DRM_INCLUDE}
    ${DRM_INCLUDE}/drm
    ${DISPLAY_EXTRA_INCLUDES}
)

target_link_libraries(egl-triangle PRIVATE
    ${EGL_LIB}
    ${GLES2_LIB}
    ${DISPLAY_EXTRA_LIBS}
)

# safemon-display
add_executable(safemon-display
    src/safemon_display.cpp
    src/gl_app.cpp
    src/drm_helper.cpp
    ${EGL_HELPER_SRC}
    src/config.cpp
)
if(PLATFORM STREQUAL "jetson")
    add_dependencies(safemon-display xdg_shell_proto)
endif()

target_include_directories(safemon-display PRIVATE
    inc
    ${HIREDIS_INCLUDE}
    ${DRM_INCLUDE}
    ${DRM_INCLUDE}/drm
    ${DISPLAY_EXTRA_INCLUDES}
)

target_link_libraries(safemon-display PRIVATE
    ${EGL_LIB}
    ${GLES2_LIB}
    ${DISPLAY_EXTRA_LIBS}
    ${HIREDIS_LIB}
)

# Install
install(TARGETS safemon-app drm-display egl-triangle safemon-display
        DESTINATION bin)