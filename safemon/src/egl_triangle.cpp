#include <iostream>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include <gbm.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

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
// Geometry — one triangle, interleaved pos(x,y) + color(r,g,b)
// ---------------------------------------------------------------------------
static const float VERTS[] = {
//   x      y      r     g     b
     0.0f,  0.8f,  1.0f, 0.2f, 0.2f,   // top    — red
    -0.7f, -0.5f,  0.2f, 1.0f, 0.2f,   // left   — green
     0.7f, -0.5f,  0.2f, 0.2f, 1.0f,   // right  — blue
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
// DRM/KMS — find connector + CRTC, present a GBM framebuffer
// ---------------------------------------------------------------------------
struct DrmState {
    int               fd       = -1;
    drmModeRes*       res      = nullptr;
    drmModeConnector* conn     = nullptr;
    uint32_t          crtc_id  = 0;
    drmModeModeInfo   mode     = {};
};

static bool drm_open(DrmState& d) {
    // card0 owns the display pipeline (VC4)
    d.fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
    if (d.fd < 0) { std::cerr << "[drm] Cannot open card0\n"; return false; }

    d.res = drmModeGetResources(d.fd);
    if (!d.res) { std::cerr << "[drm] No resources\n"; return false; }

    for (int i = 0; i < d.res->count_connectors; i++) {
        drmModeConnector* c = drmModeGetConnector(d.fd, d.res->connectors[i]);
        if (c->connection == DRM_MODE_CONNECTED && c->count_modes > 0) {
            d.conn = c;
            d.mode = c->modes[0];
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

    uint32_t W = drm.mode.hdisplay;
    uint32_t H = drm.mode.vdisplay;

    //  GBM device on renderD128 (V3D render node)
    int render_fd = open("/dev/dri/renderD128", O_RDWR | O_CLOEXEC);
    if (render_fd < 0) {
        std::cerr << "[gbm] Cannot open renderD128\n"; return 1;
    }

    gbm_device* gbm = gbm_create_device(render_fd);
    if (!gbm) { std::cerr << "[gbm] gbm_create_device failed\n"; return 1; }

    gbm_surface* gbm_surf = gbm_surface_create(
        gbm, W, H, GBM_FORMAT_XRGB8888,
        GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
    if (!gbm_surf) { std::cerr << "[gbm] Surface creation failed\n"; return 1; }

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

    const EGLint cfg_attrs[] = {
        EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
        EGL_RED_SIZE,        8,
        EGL_GREEN_SIZE,      8,
        EGL_BLUE_SIZE,       8,
        EGL_ALPHA_SIZE,      0,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };
    EGLConfig cfg;
    EGLint    n_cfg = 0;
    if (!eglChooseConfig(egl_dpy, cfg_attrs, &cfg, 1, &n_cfg) || n_cfg == 0) {
        std::cerr << "[egl] No matching config\n"; return 1;
    }

    const EGLint ctx_attrs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };
    EGLContext ctx = eglCreateContext(egl_dpy, cfg, EGL_NO_CONTEXT, ctx_attrs);
    if (ctx == EGL_NO_CONTEXT) {
        std::cerr << "[egl] eglCreateContext failed\n"; return 1;
    }

    EGLSurface egl_surf = eglCreateWindowSurface(
        egl_dpy, cfg, (EGLNativeWindowType)gbm_surf, nullptr);
    if (egl_surf == EGL_NO_SURFACE) {
        std::cerr << "[egl] eglCreateWindowSurface failed\n"; return 1;
    }

    eglMakeCurrent(egl_dpy, egl_surf, egl_surf, ctx);
    std::cout << "[gl] " << glGetString(GL_RENDERER) << "\n";

    //  OpenGL ES — build shader program
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

    // Render one frame
    glViewport(0, 0, W, H);
    glClearColor(0.07f, 0.07f, 0.10f, 1.0f);   // dark background
    glClear(GL_COLOR_BUFFER_BIT);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    eglSwapBuffers(egl_dpy, egl_surf);

    //  GBM - DRM: lock front buffer and set CRTC
    gbm_bo* bo = gbm_surface_lock_front_buffer(gbm_surf);
    if (!bo) { std::cerr << "[gbm] lock_front_buffer failed\n"; return 1; }

    uint32_t stride = gbm_bo_get_stride(bo);
    uint32_t handle = gbm_bo_get_handle(bo).u32;
    uint32_t fb_id  = 0;

    drmModeAddFB(drm.fd, W, H, 24, 32, stride, handle, &fb_id);
    drmModeSetCrtc(drm.fd, drm.crtc_id, fb_id, 0, 0,
                   &drm.conn->connector_id, 1, &drm.mode);

    std::cout << "[egl-triangle] Triangle on screen. Press Enter to exit.\n";
    std::cin.get();

    //  Cleanup
    gbm_surface_release_buffer(gbm_surf, bo);
    glDeleteBuffers(1, &vbo);
    glDeleteProgram(prog);
    eglMakeCurrent(egl_dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(egl_dpy, ctx);
    eglDestroySurface(egl_dpy, egl_surf);
    eglTerminate(egl_dpy);
    gbm_surface_destroy(gbm_surf);
    gbm_device_destroy(gbm);
    close(render_fd);
    drm_close(drm);
    return 0;
}