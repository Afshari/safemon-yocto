#include <iostream>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include "framebuffer.h"

int main() {
    // Open DRM device
    int fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
    if (fd < 0) {
        std::cerr << "Failed to open /dev/dri/card0" << std::endl;
        return 1;
    }

    // Get resources
    drmModeRes* res = drmModeGetResources(fd);
    if (!res) { std::cerr << "Failed to get DRM resources" << std::endl; return 1; }

    // Find connected connector
    drmModeConnector* conn = nullptr;
    for (int i = 0; i < res->count_connectors; i++) {
        conn = drmModeGetConnector(fd, res->connectors[i]);
        if (conn->connection == DRM_MODE_CONNECTED && conn->count_modes > 0) break;
        drmModeFreeConnector(conn);
        conn = nullptr;
    }
    if (!conn) { std::cerr << "No connected display" << std::endl; return 1; }

    uint32_t width  = conn->modes[0].hdisplay;
    uint32_t height = conn->modes[0].vdisplay;
    std::cout << "Display: " << width << "x" << height << std::endl;

    // Get CRTC
    drmModeEncoder* enc = drmModeGetEncoder(fd, conn->encoder_id);
    uint32_t crtc_id = enc->crtc_id;
    drmModeFreeEncoder(enc);

    // Create dumb buffer
    struct drm_mode_create_dumb creq;
    std::memset(&creq, 0, sizeof(creq));
    creq.width = width; creq.height = height; creq.bpp = 32;
    drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &creq);

    // Create framebuffer
    uint32_t fb_id;
    drmModeAddFB(fd, width, height, 24, 32, creq.pitch, creq.handle, &fb_id);

    // Map buffer
    struct drm_mode_map_dumb mreq;
    std::memset(&mreq, 0, sizeof(mreq));
    mreq.handle = creq.handle;
    drmIoctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &mreq);
    uint32_t* map = (uint32_t*)mmap(0, creq.size,
                    PROT_READ | PROT_WRITE, MAP_SHARED, fd, mreq.offset);

    // Create framebuffer wrapper
    Framebuffer fb(map, width, height, creq.pitch);

    // Draw status panel
    fb.fill(Color::DarkBg);

    // Title bar
    fb.draw_rect(0, 0, width, 60, Color::Gray);
    fb.draw_text(20, 20, "SAFEMON STATUS PANEL", Color::White, 3);

    // CAN status box - green = OK
    fb.draw_rect(40, 100, 300, 80, Color::Green);
    fb.draw_text(60, 125, "CAN OK", Color::Black, 3);

    // Redis status box
    fb.draw_rect(40, 200, 300, 80, Color::Green);
    fb.draw_text(60, 225, "REDIS OK", Color::Black, 3);

    // Fault box - red
    fb.draw_rect(40, 300, 300, 80, Color::Red);
    fb.draw_text(60, 325, "NO FAULTS", Color::White, 3);

    // Kernel info
    fb.draw_text(40, 420, "KERNEL 6.12 PREEMPT", Color::Yellow, 2);
    fb.draw_text(40, 450, "SAFEMON LINUX 0.1", Color::Yellow, 2);

    // Set CRTC
    drmModeSetCrtc(fd, crtc_id, fb_id, 0, 0,
                   &conn->connector_id, 1, &conn->modes[0]);

    std::cout << "Status panel displayed!" << std::endl;
    std::cout << "Press Enter to exit..." << std::endl;
    std::cin.get();

    munmap(map, creq.size);
    drmModeFreeConnector(conn);
    drmModeFreeResources(res);
    close(fd);
    return 0;
}