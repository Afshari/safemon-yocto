
#ifndef PLATFORM_JETSON

#include "egl_helper.h"

bool egl_init(EglContext& e, int drm_fd, uint32_t W, uint32_t H) {

    //  GBM  use card0 fd (same as DRM) 
    e.gbm = gbm_create_device(drm_fd);
    if (!e.gbm) { std::cerr << "[gbm] gbm_create_device failed\n"; return false; }

    e.gbm_surf = gbm_surface_create(e.gbm, W, H, GBM_FORMAT_XRGB8888,
                                GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
    if (!e.gbm_surf) { std::cerr << "[gbm] Surface creation failed\n"; return false; }

    //  EGL 
    e.dpy = eglGetPlatformDisplay(
        EGL_PLATFORM_GBM_MESA, e.gbm, nullptr);
    if (e.dpy == EGL_NO_DISPLAY) {
        std::cerr << "[egl] eglGetPlatformDisplay failed\n"; return false;
    }

    EGLint major, minor;
    if (!eglInitialize(e.dpy, &major, &minor)) {
        std::cerr << "[egl] eglInitialize failed\n"; return false;
    }
    std::cout << "[egl] EGL " << major << "." << minor << "\n";

    eglBindAPI(EGL_OPENGL_ES_API);

    // Walk all configs and pick one matching XRGB8888 + WINDOW_BIT + ES2
    EGLint num_configs = 0;
    eglGetConfigs(e.dpy, nullptr, 0, &num_configs);
    std::vector<EGLConfig> all_configs(num_configs);
    eglGetConfigs(e.dpy, all_configs.data(), num_configs, &num_configs);

    EGLConfig cfg = nullptr;
    for (int i = 0; i < num_configs; i++) {
        EGLint st, rt, vid;
        eglGetConfigAttrib(e.dpy, all_configs[i], EGL_SURFACE_TYPE,     &st);
        eglGetConfigAttrib(e.dpy, all_configs[i], EGL_RENDERABLE_TYPE,  &rt);
        eglGetConfigAttrib(e.dpy, all_configs[i], EGL_NATIVE_VISUAL_ID, &vid);
        if ((st  & EGL_WINDOW_BIT)     &&
            (rt  & EGL_OPENGL_ES2_BIT) &&
            (vid == (EGLint)GBM_FORMAT_XRGB8888)) {
            cfg = all_configs[i];
            std::cout << "[egl] Picked config " << i
                      << " vid=0x" << std::hex << vid << "\n";
            break;
        }
    }
    if (!cfg) { std::cerr << "[egl] No matching config\n"; return false; }

    const EGLint ctx_attrs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };
    e.ctx = eglCreateContext(e.dpy, cfg, EGL_NO_CONTEXT, ctx_attrs);
    if (e.ctx == EGL_NO_CONTEXT) {
        std::cerr << "[egl] eglCreateContext failed: 0x"
                  << std::hex << eglGetError() << "\n";
        return false;
    }

    e.surf = eglCreateWindowSurface(
        e.dpy, cfg, (EGLNativeWindowType)e.gbm_surf, nullptr);
    if (e.surf == EGL_NO_SURFACE) {
        std::cerr << "[egl] eglCreateWindowSurface failed: 0x"
                  << std::hex << eglGetError() << "\n";
        return false;
    }

    eglMakeCurrent(e.dpy, e.surf, e.surf, e.ctx);

    return true;
}

void egl_cleanup(EglContext& e) {
    eglMakeCurrent(e.dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(e.dpy, e.ctx);
    eglDestroySurface(e.dpy, e.surf);
    eglTerminate(e.dpy);
    gbm_surface_destroy(e.gbm_surf);
    gbm_device_destroy(e.gbm);
}

#endif // PLATFORM_JETSON