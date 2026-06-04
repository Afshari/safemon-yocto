cmake_minimum_required(VERSION 3.16)
project(safemon VERSION 0.1.0 LANGUAGES CXX)

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
find_library(GBM_LIB gbm REQUIRED)
find_path(GBM_INCLUDE gbm.h REQUIRED)

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

# egl-triangle
add_executable(egl-triangle
    src/gl_app.cpp
    src/egl_triangle.cpp
    src/drm_helper.cpp
    src/egl_helper.cpp
    src/config.cpp
)

target_include_directories(egl-triangle PRIVATE
    inc
    ${GBM_INCLUDE}
    ${DRM_INCLUDE}
    ${DRM_INCLUDE}/drm
)

target_link_libraries(egl-triangle PRIVATE
    ${EGL_LIB}
    ${GLES2_LIB}
    ${GBM_LIB}
    ${DRM_LIB}
)

# safemon-display
add_executable(safemon-display
    src/safemon_display.cpp
    src/gl_app.cpp
    src/drm_helper.cpp
    src/egl_helper.cpp
    src/config.cpp
)

target_include_directories(safemon-display PRIVATE
    inc
    ${HIREDIS_INCLUDE}
    ${GBM_INCLUDE}
    ${DRM_INCLUDE}
    ${DRM_INCLUDE}/drm
)

target_link_libraries(safemon-display PRIVATE
    ${EGL_LIB}
    ${GLES2_LIB}
    ${GBM_LIB}
    ${DRM_LIB}
    ${HIREDIS_LIB}
)

# Install
install(TARGETS safemon-app drm-display egl-triangle safemon-display
        DESTINATION bin)