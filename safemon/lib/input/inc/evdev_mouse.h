#pragma once
#include <string>
#include "iinput_device.h"

// Reads mouse events from a Linux evdev device node (e.g. /dev/input/event1).
// A single evdev mouse frame can contain REL_X, REL_Y, REL_WHEEL, and button
// events separated by EV_SYN sync markers. We accumulate relative motion and
// scroll within each sync frame and emit them as single MouseMove/MouseScroll
// events to avoid flooding the app with one-axis-at-a-time partial moves.
class EvdevMouse : public IInputDevice {
public:
    explicit EvdevMouse(std::string device_path);
    ~EvdevMouse() override;

    bool open()          override;
    void close()         override;
    bool is_open() const override;
    bool poll(std::vector<InputEvent>& out) override;

    // Translates a raw evdev button code to our MouseButton enum.
    // Exposed for testing.
    static MouseButton translate_button(uint16_t evdev_code);

protected:
    std::string m_path;
    int         m_fd = -1;

private:
    // Accumulated relative motion within the current sync frame.
    int32_t m_pending_dx     = 0;
    int32_t m_pending_dy     = 0;
    int32_t m_pending_scroll = 0;

    // Flush any accumulated motion/scroll into out, then reset accumulators.
    void flush_pending(std::vector<InputEvent>& out);
};