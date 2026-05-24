cmake_minimum_required(VERSION 3.16)
project(safemon VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# find_library(HIREDIS_LIB hiredis REQUIRED)
# find_path(HIREDIS_INCLUDE hiredis/hiredis.h REQUIRED)

# DRM test
find_library(DRM_LIB drm REQUIRED)
find_path(DRM_INCLUDE xf86drm.h REQUIRED)

# add_executable(safemon-app
#     src/can_reader.cpp
#     src/main.cpp
# )
add_executable(drm-test src/drm_test.cpp)

# target_include_directories(safemon-app PRIVATE
#     inc
#     ${HIREDIS_INCLUDE}
# )
target_include_directories(drm-test PRIVATE
    inc
    ${DRM_INCLUDE}
)

# target_link_libraries(safemon-app PRIVATE
#     ${HIREDIS_LIB}
# )
target_link_libraries(drm-test PRIVATE
    ${DRM_LIB}
)

# install(TARGETS safemon-app DESTINATION bin)
install(TARGETS drm-test DESTINATION bin)
