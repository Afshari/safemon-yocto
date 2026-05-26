#include <iostream>
#include <cstring>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <time.h>
#include <gbm.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm_fourcc.h>
#include <linux/input.h>

// ---------------------------------------------------------------------------
// Shaders
// ---------------------------------------------------------------------------
static const char* VERT_SRC = R"(
    attribute vec2 a_pos;
    attribute vec3 a_color;
    varying vec3 v_color;
    void main() {
        gl_Position = vec4(a_pos, 0.0, 1.0);
        v_color = a_color;
    }
)";

static const char* FRAG_SRC = R"(
    precision mediump float;
    varying vec3 v_color;
    void main() {
        gl_FragColor = vec4(v_color, 1.0);
    }
)";


// ---------------------------------------------------------------------------
// Geometry  one triangle, interleaved pos(x,y) + color(r,g,b)
// ---------------------------------------------------------------------------
static const float VERTS[] = {
//   x      y      r     g     b
     0.0f,  0.8f,  1.0f, 0.2f, 0.2f,   // top    red
    -0.7f, -0.5f,  0.2f, 1.0f, 0.2f,   // left   green
     0.7f, -0.5f,  0.2f, 0.2f, 1.0f,   // right  blue
};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static GLuint compile_shader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(s, sizeof(log), nullptr, log);
        std::cerr << "[gl] Shader compile error: " << log << "\n";
    }
    return s;
}

static GLuint build_program() {
    GLuint vert = compile_shader(GL_VERTEX_SHADER,   VERT_SRC);
    GLuint frag = compile_shader(GL_FRAGMENT_SHADER, FRAG_SRC);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);
    GLint ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(prog, sizeof(log), nullptr, log);
        std::cerr << "[gl] Program link error: " << log << "\n";
    }
    glDeleteShader(vert);
    glDeleteShader(frag);
    return prog;
}

// ---------------------------------------------------------------------------
// DRM/KMS
// ---------------------------------------------------------------------------
struct DrmState {
    int               fd      = -1;
    drmModeRes*       res     = nullptr;
    drmModeConnector* conn    = nullptr;
    uint32_t          crtc_id = 0;
    drmModeModeInfo   mode    = {};
};

static bool drm_open(DrmState& d) {
    d.fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
    if (d.fd < 0) { std::cerr << "[drm] Cannot open card0\n"; return false; }

    d.res = drmModeGetResources(d.fd);
    if (!d.res) { std::cerr << "[drm] No resources\n"; return false; }

    for (int i = 0; i < d.res->count_connectors; i++) {
        drmModeConnector* c = drmModeGetConnector(d.fd, d.res->connectors[i]);
        if (c->connection == DRM_MODE_CONNECTED && c->count_modes > 0) {
            d.conn = c;
            d.mode = c->modes[0]; // fallback
            for (int m = 0; m < c->count_modes; m++) {
                if (c->modes[m].hdisplay == 1920 &&
                    c->modes[m].vdisplay == 1080) {
                    d.mode = c->modes[m];
                    break;
                }
            }
            break;
        }
        drmModeFreeConnector(c);
    }
    if (!d.conn) { std::cerr << "[drm] No connected display\n"; return false; }

    drmModeEncoder* enc = drmModeGetEncoder(d.fd, d.conn->encoder_id);
    d.crtc_id = enc->crtc_id;
    drmModeFreeEncoder(enc);

    std::cout << "[drm] " << d.mode.hdisplay << "x" << d.mode.vdisplay << "\n";
    return true;
}

static void drm_close(DrmState& d) {
    if (d.conn) drmModeFreeConnector(d.conn);
    if (d.res)  drmModeFreeResources(d.res);
    if (d.fd >= 0) close(d.fd);
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
int main() {
    //  DRM 
    DrmState drm;
    if (!drm_open(drm)) return 1;
    std::cout << "[egl-triangle] version: double-buffered render loop\n";

    uint32_t W = drm.mode.hdisplay;
    uint32_t H = drm.mode.vdisplay;

    //  GBM  use card0 fd (same as DRM) 
    gbm_device* gbm = gbm_create_device(drm.fd);
    if (!gbm) { std::cerr << "[gbm] gbm_create_device failed\n"; return 1; }

    gbm_surface* gbm_surf = gbm_surface_create(gbm, W, H, GBM_FORMAT_XRGB8888,
                                GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
    if (!gbm_surf) { std::cerr << "[gbm] Surface creation failed\n"; return 1; }
    gbm_bo* prev_bo    = nullptr;
    uint32_t prev_fb_id = 0;

    //  EGL 
    EGLDisplay egl_dpy = eglGetPlatformDisplay(
        EGL_PLATFORM_GBM_MESA, gbm, nullptr);
    if (egl_dpy == EGL_NO_DISPLAY) {
        std::cerr << "[egl] eglGetPlatformDisplay failed\n"; return 1;
    }

    EGLint major, minor;
    if (!eglInitialize(egl_dpy, &major, &minor)) {
        std::cerr << "[egl] eglInitialize failed\n"; return 1;
    }
    std::cout << "[egl] EGL " << major << "." << minor << "\n";

    eglBindAPI(EGL_OPENGL_ES_API);

    // Walk all configs and pick one matching XRGB8888 + WINDOW_BIT + ES2
    EGLint num_configs = 0;
    eglGetConfigs(egl_dpy, nullptr, 0, &num_configs);
    std::vector<EGLConfig> all_configs(num_configs);
    eglGetConfigs(egl_dpy, all_configs.data(), num_configs, &num_configs);

    EGLConfig cfg = nullptr;
    for (int i = 0; i < num_configs; i++) {
        EGLint st, rt, vid;
        eglGetConfigAttrib(egl_dpy, all_configs[i], EGL_SURFACE_TYPE,     &st);
        eglGetConfigAttrib(egl_dpy, all_configs[i], EGL_RENDERABLE_TYPE,  &rt);
        eglGetConfigAttrib(egl_dpy, all_configs[i], EGL_NATIVE_VISUAL_ID, &vid);
        if ((st  & EGL_WINDOW_BIT)     &&
            (rt  & EGL_OPENGL_ES2_BIT) &&
            (vid == (EGLint)GBM_FORMAT_XRGB8888)) {
            cfg = all_configs[i];
            std::cout << "[egl] Picked config " << i
                      << " vid=0x" << std::hex << vid << "\n";
            break;
        }
    }
    if (!cfg) { std::cerr << "[egl] No matching config\n"; return 1; }

    const EGLint ctx_attrs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };
    EGLContext ctx = eglCreateContext(egl_dpy, cfg, EGL_NO_CONTEXT, ctx_attrs);
    if (ctx == EGL_NO_CONTEXT) {
        std::cerr << "[egl] eglCreateContext failed: 0x"
                  << std::hex << eglGetError() << "\n";
        return 1;
    }

    EGLSurface egl_surf = eglCreateWindowSurface(
        egl_dpy, cfg, (EGLNativeWindowType)gbm_surf, nullptr);
    if (egl_surf == EGL_NO_SURFACE) {
        std::cerr << "[egl] eglCreateWindowSurface failed: 0x"
                  << std::hex << eglGetError() << "\n";
        return 1;
    }

    eglMakeCurrent(egl_dpy, egl_surf, egl_surf, ctx);
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
    glBufferData(GL_ARRAY_BUFFER, sizeof(VERTS), VERTS, GL_STATIC_DRAW);

    glEnableVertexAttribArray(loc_pos);
    glVertexAttribPointer(loc_pos, 2, GL_FLOAT, GL_FALSE,
                          5 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(loc_color);
    glVertexAttribPointer(loc_color, 3, GL_FLOAT, GL_FALSE,
                          5 * sizeof(float), (void*)(2 * sizeof(float)));

    // FPS counter
    int        fps_count  = 0;
    double     fps_accum  = 0.0;
    struct timespec fps_last;
    clock_gettime(CLOCK_MONOTONIC, &fps_last);

    // Open keyboard input (non-blocking)
    int kbd_fd = open("/dev/input/event4", O_RDONLY | O_NONBLOCK);
    if (kbd_fd < 0)
        std::cerr << "[input] Could not open /dev/input/event0 - no keyboard input\n";

    // Render loop
    bool running = true;
    glViewport(0, 0, W, H);

    while (running) {
        // Draw
        glClearColor(0.07f, 0.07f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        eglSwapBuffers(egl_dpy, egl_surf);

        // Lock the buffer the GPU just finished rendering into
        gbm_bo* bo = gbm_surface_lock_front_buffer(gbm_surf);
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
            gbm_surface_release_buffer(gbm_surf, prev_bo);
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
            std::cout << "[fps] " << std::dec << fps_count << " fps\n";
            fps_count = 0;
            fps_accum = 0.0;
        }
    }
    std::cin.get();

    //  Cleanup 
    if (prev_bo) {
        drmModeRmFB(drm.fd, prev_fb_id);
        gbm_surface_release_buffer(gbm_surf, prev_bo);
    }
    if (kbd_fd >= 0) close(kbd_fd);
    glDeleteBuffers(1, &vbo);
    glDeleteProgram(prog);
    eglMakeCurrent(egl_dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(egl_dpy, ctx);
    eglDestroySurface(egl_dpy, egl_surf);
    eglTerminate(egl_dpy);
    gbm_surface_destroy(gbm_surf);
    gbm_device_destroy(gbm);
    drm_close(drm);
    return 0;
}