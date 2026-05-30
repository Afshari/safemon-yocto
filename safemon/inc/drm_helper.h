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

bool drm_open(DrmState& d);
void drm_close(DrmState& d);

