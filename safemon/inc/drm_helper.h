#pragma once
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

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
    d.fd = open("/dev/dri/card1", O_RDWR | O_CLOEXEC);
    if (d.fd < 0) { std::cerr << "[drm] Cannot open card1\n"; return false; }

    d.res = drmModeGetResources(d.fd);
    if (!d.res) { std::cerr << "[drm] No resources\n"; return false; }

    for (int i = 0; i < d.res->count_connectors; i++) {
        drmModeConnector* c = drmModeGetConnector(d.fd, d.res->connectors[i]);
        if (c->connection == DRM_MODE_CONNECTED && c->count_modes > 0) {
            if (c->connector_type == DRM_MODE_CONNECTOR_DSI) {
                // DSI found — use it and stop looking
                if (d.conn) drmModeFreeConnector(d.conn);
                d.conn = c;
                d.mode = c->modes[0];
                break;
            }
            // keep as fallback if no DSI found yet
            if (!d.conn) {
                d.conn = c;
                d.mode = c->modes[0];
                continue;
            }
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

