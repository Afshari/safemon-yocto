#pragma once
#include <vector>
#include <iostream>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#ifdef PLATFORM_JETSON
#include <wayland-client.h>
#include <wayland-egl.h>
#include <xdg-shell-client-protocol.h>

struct EglContext {
    wl_display*    wl_dpy       = nullptr;
    wl_compositor* wl_comp      = nullptr;
    wl_surface*    wl_surf      = nullptr;
    wl_egl_window* wl_win       = nullptr;
    xdg_wm_base*   xdg_wm       = nullptr;
    xdg_surface*   xdg_surf     = nullptr;
    xdg_toplevel*  xdg_top      = nullptr;
    EGLDisplay     dpy          = EGL_NO_DISPLAY;
    EGLContext     ctx          = EGL_NO_CONTEXT;
    EGLSurface     surf         = EGL_NO_SURFACE;
    uint32_t       width        = 0;
    uint32_t       height       = 0;
};

#else
#include <gbm.h>
#include <drm_fourcc.h>

struct EglContext {
    gbm_device*  gbm      = nullptr;
    gbm_surface* gbm_surf = nullptr;
    EGLDisplay   dpy      = EGL_NO_DISPLAY;
    EGLContext   ctx      = EGL_NO_CONTEXT;
    EGLSurface   surf     = EGL_NO_SURFACE;
};
#endif

bool egl_init(EglContext& e, int drm_fd, uint32_t W, uint32_t H);
void egl_cleanup(EglContext& e);