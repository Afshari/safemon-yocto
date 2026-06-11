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
#include "config.h"
#include "ecdsa_verify_file.h"

struct DisplayState {
    std::string last_frame  = "---";
    long        frame_count = 0;
    bool        redis_ok    = false;
    bool        can_ok      = false;
};

static volatile bool g_running = true;
static void signal_handler(int) { g_running = false; }

int main() {
    std::signal(SIGINT,  signal_handler);
    std::signal(SIGTERM, signal_handler);

    SafemonConfig cfg = load_config("/etc/safemon/safemon.conf");

    // Verify config signature before proceeding
    if (!ecdsa::verify_file("/etc/safemon/safemon.conf",
                            "/etc/safemon/pki/safemon.pub"))
    {
        std::cerr << "[startup] Config verification failed -- aborting\n";
        return 1;
    }

    // DRM init
#ifndef PLATFORM_JETSON
    DrmState drm;
    if (!drm_open(drm, cfg)) return 1;
    uint32_t W = drm.mode.hdisplay;
    uint32_t H = drm.mode.vdisplay;
#else
    // On Jetson, Weston owns the display - use fixed resolution
    // Update these values if your monitor is different
    DrmState drm;  // kept for cleanup compatibility, not used for display
    uint32_t W = 1920;
    uint32_t H = 1080;
#endif

    // EGL init
    EglContext egl;
    if (!egl_init(egl, drm.fd, W, H)) return 1;

    std::cout << "[gl-display] version: foundation\n";
    std::cout << "[gl-display] Running. Ctrl+C to exit.\n";

    // Redis
    redisContext* redis = redisConnect(cfg.redis_host.c_str(), cfg.redis_port);
    const bool redis_ok = (redis && !redis->err);
    if (!redis_ok)
        std::cerr << "[redis] Connection failed\n";
    else
        std::cout << "[redis] Connected\n";

#ifndef PLATFORM_JETSON
    gbm_bo*  prev_bo    = nullptr;
    uint32_t prev_fb_id = 0;
#endif

    glViewport(0, 0, W, H);

    GLuint rect_prog = build_rect_program();

    GLuint text_prog = build_text_program();
    GLuint font_tex  = build_font_texture();    

    while (g_running) {

        // Read from Redis
        DisplayState state;
        state.redis_ok = redis_ok;
        if (redis_ok) {
            redisReply* reply = (redisReply*)redisCommand(redis,
                                "LINDEX safemon:can:frames 0");
            if (reply && reply->type == REDIS_REPLY_STRING) {
                state.last_frame = std::string(reply->str, reply->len);
                state.can_ok     = true;
            }
            if (reply) freeReplyObject(reply);

            redisReply* cnt = (redisReply*)redisCommand(redis,
                            "LLEN safemon:can:frames");
            if (cnt && cnt->type == REDIS_REPLY_INTEGER)
                state.frame_count = cnt->integer;
            if (cnt) freeReplyObject(cnt);
        }

        // Read fault status
        std::string fault_status = "APP DOWN";
        if (redis_ok) {
            redisReply* fault = (redisReply*)redisCommand(redis,
                                "GET safemon:faults:current");
            if (fault && fault->type == REDIS_REPLY_STRING)
                fault_status = std::string(fault->str, fault->len);
            else
                fault_status = "APP DOWN";
            if (fault) freeReplyObject(fault);
        }

        // Draw
        glClearColor(0.07f, 0.07f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Title bar
        draw_rect(rect_prog, 0, 0, W, 64, 0.27f, 0.27f, 0.27f, W, H);
        draw_text_gl(text_prog, font_tex,
                    20, 18, "SAFEMON STATUS PANEL",
                    1.0f, 1.0f, 1.0f, 3.0f, W, H);

        // CAN status box
        float can_r = state.can_ok ? 0.0f : 1.0f;
        float can_g = state.can_ok ? 1.0f : 0.0f;
        draw_rect(rect_prog, 10, 90, 180, 60, can_r, can_g, 0.0f, W, H);
        draw_text_gl(text_prog, font_tex,
                    20, 108,
                    state.can_ok ? "CAN OK" : "CAN ERR",
                    0.0f, 0.0f, 0.0f, 2.0f, W, H);

        // Redis status box
        float red_r = state.redis_ok ? 0.0f : 1.0f;
        float red_g = state.redis_ok ? 1.0f : 0.0f;
        draw_rect(rect_prog, 200, 90, 180, 60, red_r, red_g, 0.0f, W, H);
        draw_text_gl(text_prog, font_tex,
                    210, 108,
                    state.redis_ok ? "REDIS OK" : "REDIS ERR",
                    0.0f, 0.0f, 0.0f, 2.0f, W, H);

        // Last frame
        draw_text_gl(text_prog, font_tex,
                    10, 175, "LAST FRAME:",
                    1.0f, 1.0f, 0.0f, 2.0f, W, H);
        draw_text_gl(text_prog, font_tex,
                    10, 200, state.last_frame.c_str(),
                    1.0f, 1.0f, 1.0f, 2.0f, W, H);

        // Frame count
        draw_text_gl(text_prog, font_tex,
                    10, 240, "FRAMES IN QUEUE:",
                    1.0f, 1.0f, 0.0f, 2.0f, W, H);
        draw_text_gl(text_prog, font_tex,
                    10, 265, std::to_string(state.frame_count).c_str(),
                    1.0f, 1.0f, 1.0f, 2.0f, W, H);

        // Fault status box
        float fr = 0.0f, fg = 0.0f, fb = 0.0f;
        if (fault_status.substr(0, 2) == "OK")
            { fr = 0.0f; fg = 1.0f; fb = 0.0f; }  // green
        else if (fault_status.substr(0, 4) == "WARN")
            { fr = 1.0f; fg = 0.8f; fb = 0.0f; }  // yellow
        else if (fault_status.substr(0, 7) == "APP DOWN")
            { fr = 0.5f; fg = 0.0f; fb = 0.5f; }       // purple - app not running
        else
            { fr = 1.0f; fg = 0.0f; fb = 0.0f; }  // red

        draw_rect(rect_prog, 10, 300, W - 20, 60, fr, fg, fb, W, H);
        draw_text_gl(text_prog, font_tex,
                    20, 318, fault_status.c_str(),
                    0.0f, 0.0f, 0.0f, 1.5f, W, H);

        // Footer
        draw_rect(rect_prog, 0, H - 30, W, 30, 0.27f, 0.27f, 0.27f, W, H);
        draw_text_gl(text_prog, font_tex,
                    10, H - 22, "KERNEL 6.12 PREEMPT | SAFEMON 0.1",
                    1.0f, 1.0f, 0.0f, 1.5f, W, H);

        eglSwapBuffers(egl.dpy, egl.surf);

#ifndef PLATFORM_JETSON
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
#endif // PLATFORM_JETSON

        sleep(1);
    }

    // Cleanup
#ifndef PLATFORM_JETSON
    if (prev_bo) {
        drmModeRmFB(drm.fd, prev_fb_id);
        gbm_surface_release_buffer(egl.gbm_surf, prev_bo);
    }
#endif
    glDeleteProgram(rect_prog);
    glDeleteProgram(text_prog);
    glDeleteTextures(1, &font_tex);
    if (redis) redisFree(redis);
    egl_cleanup(egl);
    drm_close(drm);
    return 0;
}