#pragma once
#include <string>
#include <set>
#include <stdint.h>

// Input backend selection.
// evdev   - direct /dev/input/eventX reads (RPi4, QEMU)
// wayland - wl_seat/wl_keyboard/wl_pointer via Wayland protocol (Jetson)
enum class InputBackend {
    Evdev,
    Wayland,
};

struct SafemonConfig {
    std::string          drm_device        = "/dev/dri/card1";
    std::string          redis_host        = "127.0.0.1";
    int                  redis_port        = 6379;
    std::set<uint32_t>   known_ids         = { 0x123, 0x456, 0x789 };
    int                  timeout_seconds   = 5;

    // Input devices
    std::string          keyboard_device   = "/dev/input/event0";
    std::string          mouse_device      = "/dev/input/event1";
    InputBackend         input_backend     = InputBackend::Evdev;
};

SafemonConfig load_config(const std::string& path);