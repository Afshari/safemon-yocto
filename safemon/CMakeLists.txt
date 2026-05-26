cmake_minimum_required(VERSION 3.16)
project(safemon VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#  Dependencies
find_library(HIREDIS_LIB hiredis REQUIRED)
find_path(HIREDIS_INCLUDE hiredis/hiredis.h REQUIRED)

find_library(DRM_LIB drm REQUIRED)
find_path(DRM_INCLUDE xf86drm.h REQUIRED)

find_library(EGL_LIB EGL REQUIRED)
find_library(GLES2_LIB GLESv2 REQUIRED)
find_library(GBM_LIB gbm REQUIRED)
find_path(GBM_INCLUDE gbm.h REQUIRED)

#  safemon-app  (CAN reader - Redis)
add_executable(safemon-app
    src/can_reader.cpp
    src/main.cpp
)

target_include_directories(safemon-app PRIVATE
    inc
    ${HIREDIS_INCLUDE}
)

target_link_libraries(safemon-app PRIVATE
    ${HIREDIS_LIB}
)

#  drm-display  (Redis - DRM/KMS status panel)
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

# gl-display
add_executable(gl-display
    src/gl_display.cpp
    src/gl_app.cpp
)

target_include_directories(gl-display PRIVATE
    inc
    ${HIREDIS_INCLUDE}
    ${GBM_INCLUDE}
    ${DRM_INCLUDE}
    ${DRM_INCLUDE}/drm
)

target_link_libraries(gl-display PRIVATE
    ${EGL_LIB}
    ${GLES2_LIB}
    ${GBM_LIB}
    ${DRM_LIB}
    ${HIREDIS_LIB}
)

#  Install
install(TARGETS safemon-app drm-display egl-triangle gl-display DESTINATION bin)