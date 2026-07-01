#pragma once
#include <string>
#include "iinput_device.h"

// Reads keyboard events from a Linux evdev device node (e.g. /dev/input/event0).
// Uses non-blocking I/O - poll() returns immediately if no events are pending.
class EvdevKeyboard : public IInputDevice {
public:
    explicit EvdevKeyboard(std::string device_path);
    ~EvdevKeyboard() override;

    bool open()          override;
    void close()         override;
    bool is_open() const override;
    bool poll(std::vector<InputEvent>& out) override;

    // Translates a raw evdev key code to our KeyCode enum.
    // Exposed for testing - returns KeyCode::Unknown for unmapped codes.
    static KeyCode translate_key(uint16_t evdev_code);

protected:
    std::string m_path;
    int         m_fd = -1;
};