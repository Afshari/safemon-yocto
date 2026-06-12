set(PLATFORM "rpi4" CACHE STRING "Target platform: rpi4 or jetson")
message(STATUS "Building for platform: ${PLATFORM}")

if(PLATFORM STREQUAL "jetson")
    add_compile_definitions(PLATFORM_JETSON)

    find_library(WAYLAND_CLIENT_LIB wayland-client REQUIRED)
    find_library(WAYLAND_EGL_LIB wayland-egl REQUIRED)
    find_path(WAYLAND_INCLUDE wayland-client.h REQUIRED)
    find_program(WAYLAND_SCANNER wayland-scanner REQUIRED)

    find_path(WAYLAND_PROTOCOLS_DIR xdg-shell.xml
        PATHS
            /usr/share/wayland-protocols/stable/xdg-shell
            /usr/local/share/wayland-protocols/stable/xdg-shell
        REQUIRED)

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

    add_library(xdg_shell_proto STATIC ${XDG_SHELL_SRC})
    target_include_directories(xdg_shell_proto PRIVATE ${WAYLAND_INCLUDE})
    set_target_properties(xdg_shell_proto PROPERTIES LINKER_LANGUAGE C)

    add_custom_target(xdg_shell_header DEPENDS ${XDG_SHELL_HEADER})
    add_dependencies(xdg_shell_proto xdg_shell_header)

    set_source_files_properties(
        "${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-protocol.c"
        PROPERTIES GENERATED TRUE
    )

    set(EGL_HELPER_SRC
        src/egl_helper_wayland.cpp
        "${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-protocol.c"
    )
    set(DISPLAY_EXTRA_LIBS ${WAYLAND_CLIENT_LIB} ${WAYLAND_EGL_LIB} ${DRM_LIB} xdg_shell_proto)
    set(DISPLAY_EXTRA_INCLUDES ${WAYLAND_INCLUDE} ${CMAKE_CURRENT_BINARY_DIR})
    set(PLATFORM_DEPS_TARGET xdg_shell_proto)
else()
    find_library(GBM_LIB gbm REQUIRED)
    find_path(GBM_INCLUDE gbm.h REQUIRED)

    set(EGL_HELPER_SRC src/egl_helper_gbm.cpp)
    set(DISPLAY_EXTRA_LIBS ${GBM_LIB} ${DRM_LIB})
    set(DISPLAY_EXTRA_INCLUDES ${GBM_INCLUDE} ${DRM_INCLUDE} ${DRM_INCLUDE}/drm)
    set(PLATFORM_DEPS_TARGET "")
endif()