#include <iostream>
#include <cstring>
#include <csignal>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <hiredis/hiredis.h>
#include "framebuffer.h"

// ---------------------------------------------------------------------------
// Signal handling
// ---------------------------------------------------------------------------
static volatile bool g_running = true;

static void signal_handler(int) {
    g_running = false;
}

// ---------------------------------------------------------------------------
// DRM helpers — same init sequence as drm_test, extracted into functions
// ---------------------------------------------------------------------------
struct DrmContext {
    int         fd      = -1;
    uint32_t    fb_id   = 0;
    uint32_t    crtc_id = 0;
    uint32_t*   map     = nullptr;
    size_t      map_size = 0;
    uint32_t    width   = 0;
    uint32_t    height  = 0;
    uint32_t    pitch   = 0;
    drmModeConnector* conn = nullptr;
    drmModeRes*       res  = nullptr;
};

static bool drm_init(DrmContext& ctx) {
    ctx.fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
    if (ctx.fd < 0) {
        std::cerr << "[drm] Failed to open /dev/dri/card0\n";
        return false;
    }

    ctx.res = drmModeGetResources(ctx.fd);
    if (!ctx.res) {
        std::cerr << "[drm] Failed to get DRM resources\n";
        return false;
    }

    // Find first connected connector with at least one mode
    for (int i = 0; i < ctx.res->count_connectors; i++) {
        drmModeConnector* c = drmModeGetConnector(ctx.fd, ctx.res->connectors[i]);
        if (c->connection == DRM_MODE_CONNECTED && c->count_modes > 0) {
            ctx.conn = c;
            break;
        }
        drmModeFreeConnector(c);
    }
    if (!ctx.conn) {
        std::cerr << "[drm] No connected display found\n";
        return false;
    }

    ctx.width  = ctx.conn->modes[0].hdisplay;
    ctx.height = ctx.conn->modes[0].vdisplay;
    std::cout << "[drm] Display: " << ctx.width << "x" << ctx.height << "\n";

    drmModeEncoder* enc = drmModeGetEncoder(ctx.fd, ctx.conn->encoder_id);
    ctx.crtc_id = enc->crtc_id;
    drmModeFreeEncoder(enc);

    // Create dumb buffer
    struct drm_mode_create_dumb creq;
    std::memset(&creq, 0, sizeof(creq));
    creq.width  = ctx.width;
    creq.height = ctx.height;
    creq.bpp    = 32;
    drmIoctl(ctx.fd, DRM_IOCTL_MODE_CREATE_DUMB, &creq);
    ctx.pitch    = creq.pitch;
    ctx.map_size = creq.size;

    drmModeAddFB(ctx.fd, ctx.width, ctx.height, 24, 32,
                 creq.pitch, creq.handle, &ctx.fb_id);

    struct drm_mode_map_dumb mreq;
    std::memset(&mreq, 0, sizeof(mreq));
    mreq.handle = creq.handle;
    drmIoctl(ctx.fd, DRM_IOCTL_MODE_MAP_DUMB, &mreq);

    ctx.map = (uint32_t*)mmap(nullptr, ctx.map_size,
                              PROT_READ | PROT_WRITE, MAP_SHARED,
                              ctx.fd, mreq.offset);
    if (ctx.map == MAP_FAILED) {
        std::cerr << "[drm] mmap failed\n";
        return false;
    }

    return true;
}

static void drm_cleanup(DrmContext& ctx) {
    if (ctx.map && ctx.map != MAP_FAILED)
        munmap(ctx.map, ctx.map_size);
    if (ctx.conn) drmModeFreeConnector(ctx.conn);
    if (ctx.res)  drmModeFreeResources(ctx.res);
    if (ctx.fd >= 0) close(ctx.fd);
}

// ---------------------------------------------------------------------------
// Display rendering
// ---------------------------------------------------------------------------
struct DisplayState {
    std::string last_frame   = "---";   // raw frame string from Redis
    long        frame_count  = 0;       // LLEN of the list
    bool        redis_ok     = false;
    bool        can_ok       = false;   // true when we have at least one frame
};

static void render(Framebuffer& fb, const DrmContext& ctx,
                   const DisplayState& state) {
    const uint32_t W = ctx.width;
    const uint32_t H = ctx.height;

    // Background
    fb.fill(Color::DarkBg);

    // -----------------------------------------------------------------------
    // Title bar
    // -----------------------------------------------------------------------
    fb.draw_rect(0, 0, W, 64, Color::Gray);
    fb.draw_text(20, 18, "SAFEMON STATUS PANEL", Color::White, 3);

    // -----------------------------------------------------------------------
    // Status boxes  (row 1: CAN | REDIS)
    // -----------------------------------------------------------------------
    const int BOX_W = 280;
    const int BOX_H = 70;
    const int ROW1_Y = 90;

    // CAN status
    uint32_t can_color = state.can_ok ? Color::Green : Color::Red;
    fb.draw_rect(40, ROW1_Y, BOX_W, BOX_H, can_color);
    fb.draw_text(60, ROW1_Y + 22,
                 state.can_ok ? "CAN OK" : "CAN NO DATA",
                 Color::Black, 3);

    // Redis status
    uint32_t redis_color = state.redis_ok ? Color::Green : Color::Red;
    fb.draw_rect(360, ROW1_Y, BOX_W, BOX_H, redis_color);
    fb.draw_text(380, ROW1_Y + 22,
                 state.redis_ok ? "REDIS OK" : "REDIS ERR",
                 Color::Black, 3);

    // -----------------------------------------------------------------------
    // Latest frame
    // -----------------------------------------------------------------------
    const int INFO_Y = 210;
    fb.draw_rect(40, INFO_Y, W - 80, 50, Color::Gray);
    fb.draw_text(60, INFO_Y + 14, "LAST FRAME:", Color::Yellow, 2);
    fb.draw_text(240, INFO_Y + 14, state.last_frame, Color::White, 2);

    // Frame count
    const int CNT_Y = INFO_Y + 70;
    fb.draw_text(60, CNT_Y, "FRAMES IN QUEUE:", Color::Yellow, 2);
    fb.draw_text(400, CNT_Y, std::to_string(state.frame_count), Color::White, 2);

    // -----------------------------------------------------------------------
    // Footer
    // -----------------------------------------------------------------------
    fb.draw_rect(0, H - 40, W, 40, Color::Gray);
    fb.draw_text(20, H - 26, "KERNEL 6.12 PREEMPT  |  SAFEMON LINUX 0.1", Color::Yellow, 2);

    // Push to display
    drmModeSetCrtc(ctx.fd, ctx.crtc_id, ctx.fb_id, 0, 0,
                   &ctx.conn->connector_id, 1, &ctx.conn->modes[0]);
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
int main() {
    std::signal(SIGINT,  signal_handler);
    std::signal(SIGTERM, signal_handler);

    // -- DRM init ------------------------------------------------------------
    DrmContext drm;
    if (!drm_init(drm)) return 1;

    Framebuffer fb(drm.map, drm.width, drm.height, drm.pitch);

    // -- Redis init ----------------------------------------------------------
    redisContext* redis = redisConnect("127.0.0.1", 6379);
    const bool redis_ok = (redis && !redis->err);
    if (!redis_ok)
        std::cerr << "[redis] Connection failed — display will show error state\n";
    else
        std::cout << "[redis] Connected\n";

    std::cout << "[drm_display] Running — refresh every 1s. Ctrl+C to exit.\n";

    // -- Main loop -----------------------------------------------------------
    while (g_running) {
        DisplayState state;
        state.redis_ok = redis_ok;

        if (redis_ok) {
            // Latest frame: index 0 of the list (most recent LPUSH)
            redisReply* reply =
                (redisReply*)redisCommand(redis, "LINDEX safemon:can:frames 0");

            if (reply && reply->type == REDIS_REPLY_STRING) {
                state.last_frame = std::string(reply->str, reply->len);
                state.can_ok     = true;
            }
            if (reply) freeReplyObject(reply);

            // Frame count
            redisReply* cnt =
                (redisReply*)redisCommand(redis, "LLEN safemon:can:frames");
            if (cnt && cnt->type == REDIS_REPLY_INTEGER)
                state.frame_count = cnt->integer;
            if (cnt) freeReplyObject(cnt);
        }

        render(fb, drm, state);

        sleep(1);
    }

    std::cout << "\n[drm_display] Shutting down.\n";

    if (redis) redisFree(redis);
    drm_cleanup(drm);
    return 0;
}