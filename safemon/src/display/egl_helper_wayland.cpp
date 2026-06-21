#ifdef PLATFORM_JETSON

#include "egl_helper.h"

// ------------------------------------------------------------
// xdg_wm_base ping handler - required by protocol
// ------------------------------------------------------------
static void xdg_wm_base_ping(void*, xdg_wm_base* wm, uint32_t serial)
{
    xdg_wm_base_pong(wm, serial);
}

static const xdg_wm_base_listener xdg_wm_base_listener_impl = {
    xdg_wm_base_ping
};

// ------------------------------------------------------------
// xdg_surface configure handler
// ------------------------------------------------------------
static void xdg_surface_configure(void*, xdg_surface* surf, uint32_t serial)
{
    xdg_surface_ack_configure(surf, serial);
}

static const xdg_surface_listener xdg_surface_listener_impl = {
    xdg_surface_configure
};

// ------------------------------------------------------------
// xdg_toplevel handlers
// ------------------------------------------------------------
static void xdg_toplevel_configure(void* data, xdg_toplevel*,
                                   int32_t width, int32_t height, wl_array*)
{
    EglContext* e = static_cast<EglContext*>(data);
    if (width > 0 && height > 0) {
        e->configured_width  = static_cast<uint32_t>(width);
        e->configured_height = static_cast<uint32_t>(height);
    }
}

static void xdg_toplevel_close(void*, xdg_toplevel*) {}

static const xdg_toplevel_listener xdg_toplevel_listener_impl = {
    xdg_toplevel_configure,
    xdg_toplevel_close
};

// ------------------------------------------------------------
// Registry listener
// ------------------------------------------------------------
static void registry_global(void* data, wl_registry* reg,
                             uint32_t name, const char* iface, uint32_t ver)
{
    EglContext* e = static_cast<EglContext*>(data);
    if (std::string(iface) == "wl_compositor")
        e->wl_comp = static_cast<wl_compositor*>(
            wl_registry_bind(reg, name, &wl_compositor_interface, 1));
    else if (std::string(iface) == "xdg_wm_base")
        e->xdg_wm = static_cast<xdg_wm_base*>(
            wl_registry_bind(reg, name, &xdg_wm_base_interface, 1));
}

static void registry_global_remove(void*, wl_registry*, uint32_t) {}

static const wl_registry_listener registry_listener = {
    registry_global,
    registry_global_remove
};

// ------------------------------------------------------------
// egl_init
// ------------------------------------------------------------
bool egl_init(EglContext& e, int /*drm_fd*/, uint32_t W, uint32_t H)
{
    e.width  = W;
    e.height = H;

    e.wl_dpy = wl_display_connect(nullptr);
    if (!e.wl_dpy) {
        std::cerr << "[wl] Cannot connect to Wayland display\n"
                  << "[wl] Is WAYLAND_DISPLAY set? Is Weston running?\n";
        return false;
    }
    std::cout << "[wl] Connected to Wayland display\n";

    wl_registry* reg = wl_display_get_registry(e.wl_dpy);
    wl_registry_add_listener(reg, &registry_listener, &e);
    wl_display_roundtrip(e.wl_dpy);

    if (!e.wl_comp) {
        std::cerr << "[wl] No wl_compositor found\n"; return false;
    }
    if (!e.xdg_wm) {
        std::cerr << "[wl] No xdg_wm_base found\n"; return false;
    }

    xdg_wm_base_add_listener(e.xdg_wm, &xdg_wm_base_listener_impl, nullptr);

    e.wl_surf  = wl_compositor_create_surface(e.wl_comp);
    e.xdg_surf = xdg_wm_base_get_xdg_surface(e.xdg_wm, e.wl_surf);
    xdg_surface_add_listener(e.xdg_surf, &xdg_surface_listener_impl, nullptr);

    e.xdg_top = xdg_surface_get_toplevel(e.xdg_surf);
    xdg_toplevel_add_listener(e.xdg_top, &xdg_toplevel_listener_impl, &e);
    xdg_toplevel_set_title(e.xdg_top, "safemon");
    xdg_toplevel_set_fullscreen(e.xdg_top, nullptr);

    wl_surface_commit(e.wl_surf);
    wl_display_roundtrip(e.wl_dpy);

    uint32_t actual_w = (e.configured_width  > 0) ? e.configured_width  : W;
    uint32_t actual_h = (e.configured_height > 0) ? e.configured_height : H;
    e.width  = actual_w;
    e.height = actual_h;

    e.wl_win = wl_egl_window_create(e.wl_surf, actual_w, actual_h);
    if (!e.wl_win) {
        std::cerr << "[wl] wl_egl_window_create failed\n"; return false;
    }

    e.dpy = eglGetPlatformDisplay(
        EGL_PLATFORM_WAYLAND_KHR, e.wl_dpy, nullptr);
    if (e.dpy == EGL_NO_DISPLAY) {
        std::cerr << "[egl] eglGetPlatformDisplay(WAYLAND) failed\n";
        return false;
    }

    EGLint major, minor;
    if (!eglInitialize(e.dpy, &major, &minor)) {
        std::cerr << "[egl] eglInitialize failed\n"; return false;
    }
    std::cout << "[egl] EGL " << major << "." << minor << "\n";

    eglBindAPI(EGL_OPENGL_ES_API);

    const EGLint cfg_attrs[] = {
        EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE,        8,
        EGL_GREEN_SIZE,      8,
        EGL_BLUE_SIZE,       8,
        EGL_ALPHA_SIZE,      0,
        EGL_NONE
    };
    EGLConfig cfg;
    EGLint num_cfg = 0;
    if (!eglChooseConfig(e.dpy, cfg_attrs, &cfg, 1, &num_cfg) || num_cfg == 0) {
        std::cerr << "[egl] No matching EGL config\n"; return false;
    }
    std::cout << "[egl] Picked EGL config\n";

    const EGLint ctx_attrs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };
    e.ctx = eglCreateContext(e.dpy, cfg, EGL_NO_CONTEXT, ctx_attrs);
    if (e.ctx == EGL_NO_CONTEXT) {
        std::cerr << "[egl] eglCreateContext failed: 0x"
                  << std::hex << eglGetError() << std::dec << "\n";
        return false;
    }

    e.surf = eglCreateWindowSurface(
        e.dpy, cfg, (EGLNativeWindowType)e.wl_win, nullptr);
    if (e.surf == EGL_NO_SURFACE) {
        std::cerr << "[egl] eglCreateWindowSurface failed: 0x"
                  << std::hex << eglGetError() << std::dec << "\n";
        return false;
    }

    eglMakeCurrent(e.dpy, e.surf, e.surf, e.ctx);
    std::cout << "[egl] EGL surface ready (" << actual_w << "x" << actual_h << ")\n";
    return true;
}

void egl_cleanup(EglContext& e) {
    eglMakeCurrent(e.dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(e.dpy, e.ctx);
    eglDestroySurface(e.dpy, e.surf);
    eglTerminate(e.dpy);
    if (e.wl_win)    wl_egl_window_destroy(e.wl_win);
    if (e.xdg_top)   xdg_toplevel_destroy(e.xdg_top);
    if (e.xdg_surf)  xdg_surface_destroy(e.xdg_surf);
    if (e.wl_surf)   wl_surface_destroy(e.wl_surf);
    if (e.wl_dpy)    wl_display_disconnect(e.wl_dpy);
}

#endif // PLATFORM_JETSON