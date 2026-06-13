#pragma once
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include "config.h"

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

bool drm_open(DrmState& d, const SafemonConfig& cfg);
void drm_close(DrmState& d);

