#include <iostream>
#include <csignal>
#include <GLES3/gl3.h>
#include "config.h"
#include "drm_helper.h"
#include "egl_helper.h"
#include "waterfall_data.h"
#include "waterfall_chart.h"
#include "gl_app.h"

static volatile bool g_running = true;
static void signal_handler(int) { g_running = false; }

GLuint text_prog = build_text_program();
GLuint font_tex  = build_font_texture();

int main()
{
    std::signal(SIGINT,  signal_handler);
    std::signal(SIGTERM, signal_handler);

#ifndef PLATFORM_JETSON
    SafemonConfig cfg = load_config("/etc/safemon/safemon.conf");
    DrmState drm;
    if (!drm_open(drm, cfg)) return 1;
    uint32_t W = drm.mode.hdisplay;
    uint32_t H = drm.mode.vdisplay;
#else
    DrmState drm;
    uint32_t W = 1920;
    uint32_t H = 1080;
#endif

    EglContext egl;
    if (!egl_init(egl, drm.fd, W, H)) return 1;

#ifdef PLATFORM_JETSON
    W = egl.width;
    H = egl.height;
#endif

    std::cout << "[safemon-chart] Running. Ctrl+C to exit.\n";

    // Load data
    Safemon::WaterfallData data;
    if (!data.Load("/etc/safemon/fault_data.json")) return 1;

    // Init chart
    Safemon::WaterfallChart chart;
    chart.Init(data, W, H);

    GLuint text_prog = build_text_program();
    GLuint font_tex  = build_font_texture();

    std::cout << "[gl] Vendor:   " << glGetString(GL_VENDOR)   << "\n";
    std::cout << "[gl] Renderer: " << glGetString(GL_RENDERER) << "\n";
    std::cout << "[gl] Version:  " << glGetString(GL_VERSION)  << "\n";

    glViewport(0, 0, W, H);
    glEnable(GL_DEPTH_TEST);

#ifndef PLATFORM_JETSON
    gbm_bo*  prev_bo    = nullptr;
    uint32_t prev_fb_id = 0;
#endif

    while (g_running) {
        glClearColor(0.07f, 0.07f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        chart.Render();
        chart.RenderLabels(text_prog, font_tex);

        eglSwapBuffers(egl.dpy, egl.surf);

#ifndef PLATFORM_JETSON
        gbm_bo* bo = gbm_surface_lock_front_buffer(egl.gbm_surf);
        if (!bo) break;

        uint32_t fb_id  = 0;
        uint32_t stride = gbm_bo_get_stride(bo);
        uint32_t handle = gbm_bo_get_handle(bo).u32;
        uint32_t handles[4] = { handle, 0, 0, 0 };
        uint32_t strides[4] = { stride, 0, 0, 0 };
        uint32_t offsets[4] = { 0, 0, 0, 0 };
        drmModeAddFB2(drm.fd, W, H, GBM_FORMAT_XRGB8888,
                      handles, strides, offsets, &fb_id, 0);
        drmModeSetCrtc(drm.fd, drm.crtc_id, fb_id, 0, 0,
                       &drm.conn->connector_id, 1, &drm.mode);

        if (prev_bo) {
            drmModeRmFB(drm.fd, prev_fb_id);
            gbm_surface_release_buffer(egl.gbm_surf, prev_bo);
        }
        prev_bo    = bo;
        prev_fb_id = fb_id;
#endif
    }

    // Cleanup
#ifndef PLATFORM_JETSON
    if (prev_bo) {
        drmModeRmFB(drm.fd, prev_fb_id);
        gbm_surface_release_buffer(egl.gbm_surf, prev_bo);
    }
#endif
    chart.Shutdown();
    egl_cleanup(egl);
    drm_close(drm);
    return 0;
}