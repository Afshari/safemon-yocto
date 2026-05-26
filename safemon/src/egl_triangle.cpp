#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <linux/input.h>
#include <GLES2/gl2.h>
#include <xf86drm.h>
#include <drm_fourcc.h>
#include "drm_helper.h"
#include "egl_helper.h"
#include "gl_app.h"

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
int main() {
    //  DRM 
    DrmState drm;
    if (!drm_open(drm)) return 1;
    std::cout << "[egl-triangle] version: double-buffered render loop (Refactored)\n";

    uint32_t W = drm.mode.hdisplay;
    uint32_t H = drm.mode.vdisplay;

    EglContext egl;
    if (!egl_init(egl, drm.fd, W, H)) return 1;

    gbm_bo*  prev_bo    = nullptr;
    uint32_t prev_fb_id = 0;
    
    std::cout << "[gl] Renderer: " << glGetString(GL_RENDERER) << "\n";
    std::cout << "[gl] Version:  " << glGetString(GL_VERSION)  << "\n";

    //  OpenGL ES 
    GLuint prog = build_program();
    glUseProgram(prog);

    GLint loc_pos   = glGetAttribLocation(prog, "a_pos");
    GLint loc_color = glGetAttribLocation(prog, "a_color");

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, VERTS_SIZE, VERTS, GL_STATIC_DRAW);

    glEnableVertexAttribArray(loc_pos);
    glVertexAttribPointer(loc_pos, 2, GL_FLOAT, GL_FALSE,
                          5 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(loc_color);
    glVertexAttribPointer(loc_color, VERTS_COUNT, GL_FLOAT, GL_FALSE,
                          5 * sizeof(float), (void*)(2 * sizeof(float)));

    // FPS counter
    int        fps_count  = 0;
    double     fps_accum  = 0.0;
    struct timespec fps_last;
    clock_gettime(CLOCK_MONOTONIC, &fps_last);

    // Open keyboard input (non-blocking) 
    int kbd_fd = open_keyboard();
    if (kbd_fd < 0)
        std::cerr << "[input] No keyboard found\n";

    float angle = 0.0f;
    // Render loop
    bool running = true;
    glViewport(0, 0, W, H);

    while (running) {

        angle += 0.016f;   // roughly 1 degree per frame at 60fps
        set_rotation(prog, angle);
        // Draw
        glClearColor(0.07f, 0.07f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, VERTS_COUNT);
        eglSwapBuffers(egl.dpy, egl.surf);

        // Lock the buffer the GPU just finished rendering into
        gbm_bo* bo = gbm_surface_lock_front_buffer(egl.gbm_surf);
        if (!bo) { std::cerr << "[gbm] lock_front_buffer failed\n"; break; }

        // Register it with DRM
        uint32_t fb_id  = 0;
        uint32_t stride = gbm_bo_get_stride(bo);
        uint32_t handle = gbm_bo_get_handle(bo).u32;
        uint32_t handles[4] = { handle, 0, 0, 0 };
        uint32_t strides[4] = { stride, 0, 0, 0 };
        uint32_t offsets[4] = { 0, 0, 0, 0 };
        drmModeAddFB2(drm.fd, W, H, GBM_FORMAT_XRGB8888,
                    handles, strides, offsets, &fb_id, 0);

        // Put it on screen
        drmModeSetCrtc(drm.fd, drm.crtc_id, fb_id, 0, 0,
                    &drm.conn->connector_id, 1, &drm.mode);

        // Now safe to release the previous frame's buffer
        if (prev_bo) {
            drmModeRmFB(drm.fd, prev_fb_id);
            gbm_surface_release_buffer(egl.gbm_surf, prev_bo);
        }

        prev_bo    = bo;
        prev_fb_id = fb_id;

        // Poll keyboard
        if (kbd_fd >= 0) {
            struct input_event ev;
            while (read(kbd_fd, &ev, sizeof(ev)) == sizeof(ev)) {
                if (ev.type == EV_KEY &&
                    ev.code == KEY_ESC &&
                    ev.value == 1) {   // 1 = key down
                    running = false;
                }
            }
        }

        // FPS measurement
        struct timespec fps_now;
        clock_gettime(CLOCK_MONOTONIC, &fps_now);
        double elapsed = (fps_now.tv_sec  - fps_last.tv_sec) +
                        (fps_now.tv_nsec - fps_last.tv_nsec) * 1e-9;
        fps_accum += elapsed;
        fps_count++;
        fps_last = fps_now;

        if (fps_accum >= 1.0) {
            // std::cout << "[fps] " << std::dec << fps_count << " fps\n";
            fps_count = 0;
            fps_accum = 0.0;
        }
    }

    //  Cleanup 
    if (prev_bo) {
        drmModeRmFB(drm.fd, prev_fb_id);
        gbm_surface_release_buffer(egl.gbm_surf, prev_bo);
    }
    if (kbd_fd >= 0) close(kbd_fd);
    glDeleteBuffers(1, &vbo);
    glDeleteProgram(prog);
    egl_cleanup(egl);    
    drm_close(drm);
    return 0;
}