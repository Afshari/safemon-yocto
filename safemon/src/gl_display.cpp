#include <iostream>
#include <csignal>
#include <fcntl.h>
#include <unistd.h>
#include <GLES2/gl2.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm_fourcc.h>
#include <hiredis/hiredis.h>
#include "drm_helper.h"
#include "egl_helper.h"
#include "gl_app.h"

static volatile bool g_running = true;
static void signal_handler(int) { g_running = false; }

int main() {
    std::signal(SIGINT,  signal_handler);
    std::signal(SIGTERM, signal_handler);

    // DRM init
    DrmState drm;
    if (!drm_open(drm)) return 1;

    uint32_t W = drm.mode.hdisplay;
    uint32_t H = drm.mode.vdisplay;

    // EGL init
    EglContext egl;
    if (!egl_init(egl, drm.fd, W, H)) return 1;

    std::cout << "[gl-display] version: foundation\n";
    std::cout << "[gl-display] Running. Ctrl+C to exit.\n";

    gbm_bo*  prev_bo    = nullptr;
    uint32_t prev_fb_id = 0;

    glViewport(0, 0, W, H);

    GLuint rect_prog = build_rect_program();

    GLuint text_prog = build_text_program();
    GLuint font_tex  = build_font_texture();    

    while (g_running) {
        // Clear to dark background
        glClearColor(0.07f, 0.07f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // title bar - gray
        draw_rect(rect_prog, 0, 0, W, 64, 0.27f, 0.27f, 0.27f, W, H);
        // CAN OK box - green
        draw_rect(rect_prog, 40, 90, 280, 70, 0.0f, 1.0f, 0.0f, W, H);
        // REDIS OK box - green  
        draw_rect(rect_prog, 360, 90, 280, 70, 0.0f, 1.0f, 0.0f, W, H);
        // footer - gray
        draw_rect(rect_prog, 0, H - 40, W, 40, 0.27f, 0.27f, 0.27f, W, H);
        
        draw_text_gl(text_prog, font_tex,
             20, 18, "SAFEMON STATUS PANEL",
             1.0f, 1.0f, 1.0f, 3.0f, W, H);

        eglSwapBuffers(egl.dpy, egl.surf);

        // GBM -> DRM
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

        sleep(1);
    }

    // Cleanup
    if (prev_bo) {
        drmModeRmFB(drm.fd, prev_fb_id);
        gbm_surface_release_buffer(egl.gbm_surf, prev_bo);
    }
    glDeleteProgram(rect_prog);
    glDeleteProgram(text_prog);
    glDeleteTextures(1, &font_tex);
    egl_cleanup(egl);
    drm_close(drm);
    return 0;
}