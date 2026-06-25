#include <iostream>
#include <csignal>
#include <GLES3/gl3.h>
#include <hiredis/hiredis.h>
#include <time.h>
#include <fstream>
#include <memory>
#include "config.h"
#include "drm_helper.h"
#include "egl_helper.h"
#include "ecdsa_verify_file.h"
#include "dashboard.h"
#include "text_renderer.h"
#include "display_reader.h"
#include "redis_client_impl.h"

// ---------------------------------------------------------------------------
// Helpers that stay here (display/system, not Redis logic)
// ---------------------------------------------------------------------------

static std::string format_clock()
{
    time_t now = time(nullptr);
    struct tm tm_now;
    localtime_r(&now, &tm_now);
    char buf[16];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d",
             tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec);
    return std::string(buf);
}

static std::string format_system_uptime()
{
    std::ifstream f("/proc/uptime");
    double uptime_seconds = 0;
    if (f.is_open()) f >> uptime_seconds;

    long total = static_cast<long>(uptime_seconds);
    long h = total / 3600;
    long m = (total % 3600) / 60;
    long s = total % 60;
    char buf[16];
    snprintf(buf, sizeof(buf), "%02ld:%02ld:%02ld", h, m, s);
    return std::string(buf);
}

static float get_time_seconds()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<float>(ts.tv_sec) +
           static_cast<float>(ts.tv_nsec) / 1e9f;
}

static std::string shorten_machine_name(const std::string& machine)
{
    if (machine.find("raspberrypi4") != std::string::npos) return "RPI4";
    if (machine.find("qemuarm")      != std::string::npos) return "QEMU";
    if (machine.find("jetson")       != std::string::npos) return "JETSON";
    return machine;
}

struct BuildInfo {
    std::string build_date = "unknown";
    std::string machine    = "unknown";
};

static BuildInfo load_build_info()
{
    BuildInfo info;
    std::ifstream f("/etc/safemon-build-info");
    if (!f.is_open()) return info;

    std::string line;
    while (std::getline(f, line)) {
        auto pos = line.find(": ");
        if (pos == std::string::npos) continue;
        std::string key = line.substr(0, pos);
        std::string val = line.substr(pos + 2);
        if (key == "Build date") info.build_date = val;
        else if (key == "Machine") info.machine = shorten_machine_name(val);
    }
    return info;
}

// ---------------------------------------------------------------------------
// Signal handler
// ---------------------------------------------------------------------------

static volatile bool g_running = true;
static void signal_handler(int) { g_running = false; }

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    std::signal(SIGINT,  signal_handler);
    std::signal(SIGTERM, signal_handler);

    SafemonConfig cfg = load_config("/etc/safemon/safemon.conf");

    if (!ecdsa::verify_file("/etc/safemon/safemon.conf",
                            "/etc/safemon/pki/safemon.pub"))
    {
        std::cerr << "[startup] Config verification failed -- aborting\n";
        return 1;
    }

#ifndef PLATFORM_JETSON
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

    std::cout << "[safemon-display] version: glass\n";
    std::cout << "[safemon-display] Running. Ctrl+C to exit.\n";

    redisContext* redis = redisConnect(cfg.redis_host.c_str(), cfg.redis_port);
    bool redis_ok = (redis && !redis->err);
    if (!redis_ok)
        std::cerr << "[redis] Connection failed\n";
    else
        std::cout << "[redis] Connected\n";

    // DisplayStateReader owns its own RedisClient
    DisplayStateReader display_reader(
        std::make_unique<RedisClient>(cfg.redis_host, cfg.redis_port)
    );

    Safemon::TextRenderer textRenderer;
    textRenderer.Init("/etc/safemon/JetBrainsMono-Regular.ttf");

    float design_scale = std::min(
        static_cast<float>(W) / 800.0f,
        static_cast<float>(H) / 480.0f
    );

    Safemon::Dashboard dashboard;
    dashboard.Init(&textRenderer, design_scale);

    std::cout << "[gl] Vendor:   " << glGetString(GL_VENDOR)   << "\n";
    std::cout << "[gl] Renderer: " << glGetString(GL_RENDERER) << "\n";
    std::cout << "[gl] Version:  " << glGetString(GL_VERSION)  << "\n";

    glViewport(0, 0, W, H);

#ifndef PLATFORM_JETSON
    gbm_bo*  prev_bo    = nullptr;
    uint32_t prev_fb_id = 0;
#endif

    BuildInfo build_info = load_build_info();

    while (g_running) {

        // Redis state via DisplayStateReader
        DashboardState state = display_reader.read(redis_ok);

        // System state (display-side, not Redis)
        state.clock          = format_clock();
        state.uptime         = format_system_uptime();
        state.footer_machine = build_info.machine;
        state.footer_build   = build_info.build_date;

        glClearColor(0.07f, 0.11f, 0.21f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float time_seconds = get_time_seconds();
        dashboard.Render(state, time_seconds, W, H);

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

        sleep(1);
    }

#ifndef PLATFORM_JETSON
    if (prev_bo) {
        drmModeRmFB(drm.fd, prev_fb_id);
        gbm_surface_release_buffer(egl.gbm_surf, prev_bo);
    }
#endif
    dashboard.Shutdown();
    textRenderer.Shutdown();
    if (redis) redisFree(redis);
    egl_cleanup(egl);
    drm_close(drm);
    return 0;
}