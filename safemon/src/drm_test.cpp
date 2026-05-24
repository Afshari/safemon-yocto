#include <iostream>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

int main() {
    // Open DRM device
    int fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
    if (fd < 0) {
        std::cerr << "Failed to open /dev/dri/card0" << std::endl;
        return 1;
    }
    std::cout << "Opened DRM device" << std::endl;

    // Get DRM resources
    drmModeRes* res = drmModeGetResources(fd);
    if (!res) {
        std::cerr << "Failed to get DRM resources" << std::endl;
        return 1;
    }

    // Find connected connector
    drmModeConnector* conn = nullptr;
    for (int i = 0; i < res->count_connectors; i++) {
        conn = drmModeGetConnector(fd, res->connectors[i]);
        if (conn->connection == DRM_MODE_CONNECTED && conn->count_modes > 0)
            break;
        drmModeFreeConnector(conn);
        conn = nullptr;
    }

    if (!conn) {
        std::cerr << "No connected display found" << std::endl;
        return 1;
    }
    std::cout << "Found connected display!" << std::endl;
    std::cout << "Resolution: "
              << conn->modes[0].hdisplay << "x"
              << conn->modes[0].vdisplay << std::endl;

    // Get encoder and CRTC
    drmModeEncoder* enc = drmModeGetEncoder(fd, conn->encoder_id);
    uint32_t crtc_id = enc->crtc_id;
    drmModeFreeEncoder(enc);

    // Create dumb buffer
    struct drm_mode_create_dumb creq;
    std::memset(&creq, 0, sizeof(creq));
    creq.width  = conn->modes[0].hdisplay;
    creq.height = conn->modes[0].vdisplay;
    creq.bpp    = 32;
    drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &creq);

    // Create framebuffer
    uint32_t fb_id;
    drmModeAddFB(fd, creq.width, creq.height, 24, 32,
                 creq.pitch, creq.handle, &fb_id);

    // Map buffer
    struct drm_mode_map_dumb mreq;
    std::memset(&mreq, 0, sizeof(mreq));
    mreq.handle = creq.handle;
    drmIoctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &mreq);

    uint32_t* map = (uint32_t*)mmap(0, creq.size,
                    PROT_READ | PROT_WRITE, MAP_SHARED,
                    fd, mreq.offset);

    // Fill screen with green color
    std::cout << "Filling screen with green..." << std::endl;
    for (uint64_t i = 0; i < creq.size / 4; i++)
        map[i] = 0x0000FF00;  // green (ARGB)

    // Set CRTC
    drmModeSetCrtc(fd, crtc_id, fb_id, 0, 0,
                   &conn->connector_id, 1, &conn->modes[0]);

    std::cout << "Screen should be green now!" << std::endl;
    std::cout << "Press Enter to exit..." << std::endl;
    std::cin.get();

    // Cleanup
    munmap(map, creq.size);
    drmModeFreeConnector(conn);
    drmModeFreeResources(res);
    close(fd);
    return 0;
}