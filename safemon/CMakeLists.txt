cmake_minimum_required(VERSION 3.16)
project(safemon VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(cmake/dependencies.cmake)
include(cmake/platform.cmake)

add_subdirectory(lib)

# proto generated sources
set(PROTO_SRC
    proto/fault.pb.cc
    proto/fault.grpc.pb.cc
)

# ---------------------------------------------------------------------------
# safemon-app
# ---------------------------------------------------------------------------
add_executable(safemon-app
    src/can_reader.cpp
    src/main.cpp
    src/grpc_server.cpp
    ${PROTO_SRC}
)

target_include_directories(safemon-app PRIVATE
    inc
    ${HIREDIS_INCLUDE}
    ${Protobuf_INCLUDE_DIRS}
    proto
)

target_link_libraries(safemon-app PRIVATE
    config
    fault_detector
    ecdsa
    ${HIREDIS_LIB}
    protobuf::libprotobuf
    gRPC::grpc++
    ${ABSEIL_LIBS}
)

# ---------------------------------------------------------------------------
# drm-display
# ---------------------------------------------------------------------------
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

# ---------------------------------------------------------------------------
# egl-triangle
# ---------------------------------------------------------------------------
add_executable(egl-triangle
    src/gl_app.cpp
    src/egl_triangle.cpp
    src/drm_helper.cpp
    ${EGL_HELPER_SRC}
)
if(PLATFORM_DEPS_TARGET)
    add_dependencies(egl-triangle ${PLATFORM_DEPS_TARGET})
endif()

target_include_directories(egl-triangle PRIVATE
    inc
    ${DRM_INCLUDE}
    ${DRM_INCLUDE}/drm
    ${DISPLAY_EXTRA_INCLUDES}
)

target_link_libraries(egl-triangle PRIVATE
    config
    ${EGL_LIB}
    ${GLES2_LIB}
    ${DISPLAY_EXTRA_LIBS}
)

# ---------------------------------------------------------------------------
# safemon-display
# ---------------------------------------------------------------------------
add_executable(safemon-display
    src/safemon_display.cpp
    src/gl_app.cpp
    src/drm_helper.cpp
    ${EGL_HELPER_SRC}
)
if(PLATFORM_DEPS_TARGET)
    add_dependencies(safemon-display ${PLATFORM_DEPS_TARGET})
endif()

target_include_directories(safemon-display PRIVATE
    inc
    ${HIREDIS_INCLUDE}
    ${DRM_INCLUDE}
    ${DRM_INCLUDE}/drm
    ${DISPLAY_EXTRA_INCLUDES}
)

target_link_libraries(safemon-display PRIVATE
    config
    ecdsa
    ${EGL_LIB}
    ${GLES2_LIB}
    ${DISPLAY_EXTRA_LIBS}
    ${HIREDIS_LIB}
)

# ---------------------------------------------------------------------------
# Install
# ---------------------------------------------------------------------------
install(TARGETS safemon-app drm-display egl-triangle safemon-display
        DESTINATION bin)