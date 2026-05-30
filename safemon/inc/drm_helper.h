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
            d.conn = c;
            d.mode = c->modes[0]; // fallback
            for (int m = 0; m < c->count_modes; m++) {
                if ((c->modes[m].hdisplay == 800  &&
                    c->modes[m].vdisplay == 480) ||
                    (c->modes[m].hdisplay == 1920 &&
                    c->modes[m].vdisplay == 1080)) {
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

