#pragma once
#include <vector>
#include <iostream>
#include <gbm.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <drm_fourcc.h>

struct EglContext {
    gbm_device*  gbm      = nullptr;
    gbm_surface* gbm_surf = nullptr;
    EGLDisplay   dpy      = EGL_NO_DISPLAY;
    EGLContext   ctx      = EGL_NO_CONTEXT;
    EGLSurface   surf     = EGL_NO_SURFACE;
};

bool egl_init(EglContext& e, int drm_fd, uint32_t W, uint32_t H);
void egl_cleanup(EglContext& e);