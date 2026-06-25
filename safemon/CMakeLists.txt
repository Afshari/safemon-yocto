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
    src/services/safemon_app.cpp
    ${PROTO_SRC}
)

target_include_directories(safemon-app PRIVATE
    inc/services
    lib/can_bridge/inc
    lib/common/inc
    lib/grpc_server/inc
    ${HIREDIS_INCLUDE}
    ${Protobuf_INCLUDE_DIRS}
    proto
)

target_link_libraries(safemon-app PRIVATE
    config
    fault_detector
    can_bridge
    grpc_server
    ecdsa
    ${HIREDIS_LIB}
    protobuf::libprotobuf
    gRPC::grpc++
    ${ABSEIL_LIBS}
)

target_include_directories(grpc_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/proto
)

# ---------------------------------------------------------------------------
# drm-display (legacy)
# ---------------------------------------------------------------------------
add_executable(drm-display
    src/legacy/drm_display.cpp
    src/legacy/framebuffer.cpp
)

target_include_directories(drm-display PRIVATE
    inc/legacy
    ${HIREDIS_INCLUDE}
    ${DRM_INCLUDE}
    ${DRM_INCLUDE}/drm
)

target_link_libraries(drm-display PRIVATE
    ${DRM_LIB}
    ${HIREDIS_LIB}
)

# ---------------------------------------------------------------------------
# egl-triangle (legacy)
# ---------------------------------------------------------------------------
add_executable(egl-triangle
    src/display/gl_app.cpp
    src/legacy/egl_triangle.cpp
    src/display/drm_helper.cpp
    ${EGL_HELPER_SRC}
)
if(PLATFORM_DEPS_TARGET)
    add_dependencies(egl-triangle ${PLATFORM_DEPS_TARGET})
endif()

target_include_directories(egl-triangle PRIVATE
    inc/display
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
    src/display/safemon_display.cpp
    src/display/glass_panel.cpp
    src/display/dashboard.cpp
    src/display/text_renderer.cpp
    src/display/drm_helper.cpp
    ${EGL_HELPER_SRC}
)
if(PLATFORM_DEPS_TARGET)
    add_dependencies(safemon-display ${PLATFORM_DEPS_TARGET})
endif()

target_include_directories(safemon-display PRIVATE
    inc/display
    inc/third_party
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
# safemon-chart
# ---------------------------------------------------------------------------
add_executable(safemon-chart
    src/display/safemon_chart.cpp
    src/display/waterfall_data.cpp
    src/display/waterfall_chart.cpp
    src/display/camera.cpp
    src/display/text_renderer.cpp
    src/display/axis_gizmo.cpp
    src/display/drm_helper.cpp
    ${EGL_HELPER_SRC}
)
if(PLATFORM_DEPS_TARGET)
    add_dependencies(safemon-chart ${PLATFORM_DEPS_TARGET})
endif()

target_include_directories(safemon-chart PRIVATE
    inc/display
    inc/third_party
    ${HIREDIS_INCLUDE}
    ${DRM_INCLUDE}
    ${DRM_INCLUDE}/drm
    ${DISPLAY_EXTRA_INCLUDES}
)

target_link_libraries(safemon-chart PRIVATE
    config
    ${EGL_LIB}
    ${GLES2_LIB}
    ${DISPLAY_EXTRA_LIBS}
    ${HIREDIS_LIB}
)

# ---------------------------------------------------------------------------
# Install
# ---------------------------------------------------------------------------
install(TARGETS safemon-app drm-display egl-triangle safemon-display safemon-chart
        DESTINATION bin)